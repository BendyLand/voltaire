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
#include "scene/3d/physics/collision_object_3d.h"
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

#include "scene/3d/visual_instance_3d.h"

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

#include "scene/3d/visual_instance_3d.h"

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
#include "scene/3d/visual_instance_3d.h"
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
#include "scene/3d/visual_instance_3d.h"
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
// --- Inlined from scene/3d/fog_volume.h ---
#include "core/templates/rid.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/material.h"

class FogVolume : public Node3D
{
public:
	RenderInstance3D render_instance;

private:
	Vector3 size = Vector3(2, 2, 2);
	Ref<Material> material;
	RSE::FogVolumeShape shape = RSE::FOG_VOLUME_SHAPE_BOX;

	RID volume;

protected:
	_FORCE_INLINE_ RID _get_volume() { return volume; }
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	Vector3 get_size() const;

	void set_shape(RSE::FogVolumeShape p_type);
	RSE::FogVolumeShape get_shape() const;

	Ref<Material> get_material() const;

	AABB get_aabb() const;
	PackedStringArray get_configuration_warnings() const override;

	FogVolume();
	~FogVolume();
};

// --- End scene/3d/fog_volume.h ---
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
#include "scene/3d/visual_instance_3d.h"
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
// --- Inlined from scene/3d/visible_on_screen_notifier_3d.h ---
#include "scene/3d/visual_instance_3d.h"

class VisibleOnScreenNotifier3D : public Node3D
{
public:
	enum EnableMode
	{
		ENABLE_MODE_INHERIT,
		ENABLE_MODE_ALWAYS,
		ENABLE_MODE_WHEN_PAUSED,
	};

	RenderInstance3D render_instance;

private:
	AABB aabb = AABB(Vector3(-1, -1, -1), Vector3(2, 2, 2));
	bool on_screen = false;
	bool enabler_enabled = false;
	EnableMode enable_mode = ENABLE_MODE_INHERIT;
	NodePath enable_node_path = NodePath("..");

	void _visibility_enter();
	void _visibility_exit();
	void _update_enable_mode(bool p_enable);

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_aabb(const AABB& p_aabb);
	AABB get_aabb() const;
	bool is_on_screen() const;

	void set_enabler_enabled(bool p_enabled);
	bool is_enabler_enabled() const { return enabler_enabled; }

	void set_enable_mode(EnableMode p_mode);
	EnableMode get_enable_mode() const { return enable_mode; }

	void set_enable_node_path(const NodePath& p_path);
	NodePath get_enable_node_path() const { return enable_node_path; }

	VisibleOnScreenNotifier3D(bool p_enabler = false);
	~VisibleOnScreenNotifier3D();
};

// --- End scene/3d/visible_on_screen_notifier_3d.h ---

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

// ============================================================================
// Merged from scene/3d/cpu_particles_3d.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/math/random_number_generator.h"
// --- Inlined from scene/3d/cpu_particles_3d.h ---
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/curve.h"
#include "scene/resources/gradient.h"

class Mesh;
class RandomNumberGenerator;

class CPUParticles3D : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum DrawOrder
	{
		DRAW_ORDER_INDEX,
		DRAW_ORDER_LIFETIME,
		DRAW_ORDER_VIEW_DEPTH,
		DRAW_ORDER_MAX
	};

	enum Parameter
	{
		PARAM_INITIAL_LINEAR_VELOCITY,
		PARAM_ANGULAR_VELOCITY,
		PARAM_ORBIT_VELOCITY,
		PARAM_LINEAR_ACCEL,
		PARAM_RADIAL_ACCEL,
		PARAM_TANGENTIAL_ACCEL,
		PARAM_DAMPING,
		PARAM_ANGLE,
		PARAM_SCALE,
		PARAM_HUE_VARIATION,
		PARAM_ANIM_SPEED,
		PARAM_ANIM_OFFSET,
		PARAM_MAX
	};

	enum ParticleFlags
	{
		PARTICLE_FLAG_ALIGN_Y_TO_VELOCITY,
		PARTICLE_FLAG_ROTATE_Y,
		PARTICLE_FLAG_DISABLE_Z,
		PARTICLE_FLAG_MAX
	};

	enum EmissionShape
	{
		EMISSION_SHAPE_POINT,
		EMISSION_SHAPE_SPHERE,
		EMISSION_SHAPE_SPHERE_SURFACE,
		EMISSION_SHAPE_BOX,
		EMISSION_SHAPE_POINTS,
		EMISSION_SHAPE_DIRECTED_POINTS,
		EMISSION_SHAPE_RING,
		EMISSION_SHAPE_MAX
	};

private:
	bool emitting = false;
	bool active = false;

	struct Particle
	{
		Transform3D transform;
		Color color;
		real_t custom[4] = {};
		Vector3 velocity;
		bool active = false;
		real_t angle_rand = 0.0;
		real_t scale_rand = 0.0;
		real_t hue_rot_rand = 0.0;
		real_t anim_offset_rand = 0.0;
		Color start_color_rand;
		double time = 0.0;
		double lifetime = 0.0;
		Color base_color;

		uint32_t seed = 0;
	};

	double time = 0.0;
	double frame_remainder = 0.0;
	int cycle = 0;
	bool redraw = false;

	RID multimesh;

	Vector<Particle> particles;
	Vector<float> particle_data;
	Vector<int> particle_order;

	struct SortLifetime
	{
		const Particle* particles = nullptr;

		bool operator()(int p_a, int p_b) const
		{
			return particles[p_a].time > particles[p_b].time;
		}
	};

	struct SortAxis
	{
		const Particle* particles = nullptr;
		Vector3 axis;

		bool operator()(int p_a, int p_b) const
		{
			return axis.dot(particles[p_a].transform.origin) <
				   axis.dot(particles[p_b].transform.origin);
		}
	};

	//

	bool one_shot = false;

	double lifetime = 1.0;
	double pre_process_time = 0.0;
	real_t _request_process_time = 0.0;
	real_t _request_process_time_residual = 0.0;
	real_t explosiveness_ratio = 0.0;
	real_t randomness_ratio = 0.0;
	double lifetime_randomness = 0.0;
	double speed_scale = 1.0;
	AABB visibility_aabb;
	bool local_coords = false;
	int fixed_fps = 0;
	bool fractional_delta = true;
	uint32_t seed = 0;
	bool use_fixed_seed = false;

	Transform3D inv_emission_transform;

	SafeFlag can_update;

	DrawOrder draw_order = DRAW_ORDER_INDEX;

	Ref<Mesh> mesh;

	////////

	Vector3 direction = Vector3(1, 0, 0);
	real_t spread = 45.0;
	real_t flatness = 0.0;

	real_t parameters_min[PARAM_MAX] = {};
	real_t parameters_max[PARAM_MAX] = {};

	Ref<Curve> curve_parameters[PARAM_MAX];
	Color color = Color(1, 1, 1, 1);
	Ref<Gradient> color_ramp;
	Ref<Gradient> color_initial_ramp;

	bool particle_flags[PARTICLE_FLAG_MAX] = {};

	EmissionShape emission_shape = EMISSION_SHAPE_POINT;
	real_t emission_sphere_radius = 1.0;
	Vector3 emission_box_extents = Vector3(1, 1, 1);
	Vector<Vector3> emission_points;
	Vector<Vector3> emission_normals;
	Vector<Color> emission_colors;
	Vector3 emission_ring_axis;
	real_t emission_ring_height = 0.0;
	real_t emission_ring_radius = 0.0;
	real_t emission_ring_inner_radius = 0.0;
	real_t emission_ring_cone_angle = 0.0;

	Ref<Curve> scale_curve_x;
	Ref<Curve> scale_curve_y;
	Ref<Curve> scale_curve_z;
	bool split_scale = false;

	Vector3 gravity = Vector3(0, -9.8, 0);

	Ref<RandomNumberGenerator> rng;

	void _update_internal();
	void _particles_process(double p_delta);
	void _update_particle_data_buffer();
	void _set_emitting();

	Mutex update_mutex;

	void _update_render_thread();

	void _set_redraw(bool p_redraw);

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	AABB get_aabb() const;

	void set_emitting(bool p_emitting);
	void set_amount(int p_amount);
	void set_lifetime(double p_lifetime);
	void set_one_shot(bool p_one_shot);
	void set_pre_process_time(double p_time);
	void set_explosiveness_ratio(real_t p_ratio);
	void set_randomness_ratio(real_t p_ratio);
	void set_lifetime_randomness(double p_random);
	void set_use_local_coordinates(bool p_enable);
	void set_speed_scale(double p_scale);

	bool is_emitting() const;
	int get_amount() const;
	double get_lifetime() const;
	bool get_one_shot() const;
	double get_pre_process_time() const;
	real_t get_explosiveness_ratio() const;
	real_t get_randomness_ratio() const;
	AABB get_visibility_aabb() const;
	double get_lifetime_randomness() const;
	bool get_use_local_coordinates() const;
	double get_speed_scale() const;

	void set_fixed_fps(int p_count);
	int get_fixed_fps() const;

	void set_fractional_delta(bool p_enable);
	bool get_fractional_delta() const;

	void set_draw_order(DrawOrder p_order);
	DrawOrder get_draw_order() const;

	Ref<Mesh> get_mesh() const;

	void set_use_fixed_seed(bool p_use_fixed_seed = false);
	bool get_use_fixed_seed() const;

	void set_seed(uint32_t p_seed);
	uint32_t get_seed() const;

	void request_particles_process(
		real_t p_requested_process_time, real_t p_request_process_time_residual = 0.0f);

	///////////////////

	void set_direction(Vector3 p_direction);
	Vector3 get_direction() const;

	void set_spread(real_t p_spread);
	real_t get_spread() const;

	void set_flatness(real_t p_flatness);
	real_t get_flatness() const;

	real_t get_param_min(Parameter p_param) const;

	real_t get_param_max(Parameter p_param) const;

	Ref<Curve> get_param_curve(Parameter p_param) const;

	void set_color(const Color& p_color);
	Color get_color() const;

	void set_color_ramp(const Ref<Gradient>& p_ramp);
	Ref<Gradient> get_color_ramp() const;

	void set_color_initial_ramp(const Ref<Gradient>& p_ramp);
	Ref<Gradient> get_color_initial_ramp() const;

	void set_particle_flag(ParticleFlags p_particle_flag, bool p_enable);
	bool get_particle_flag(ParticleFlags p_particle_flag) const;

	void set_emission_points(const Vector<Vector3>& p_points);
	void set_emission_normals(const Vector<Vector3>& p_normals);
	void set_emission_colors(const Vector<Color>& p_colors);
	void set_scale_curve_x(Ref<Curve> p_scale_curve);
	void set_scale_curve_y(Ref<Curve> p_scale_curve);
	void set_scale_curve_z(Ref<Curve> p_scale_curve);
	void set_split_scale(bool p_split_scale);

	EmissionShape get_emission_shape() const;
	real_t get_emission_sphere_radius() const;
	Vector3 get_emission_box_extents() const;
	Vector<Vector3> get_emission_points() const;
	Vector<Vector3> get_emission_normals() const;
	Vector<Color> get_emission_colors() const;
	Vector3 get_emission_ring_axis() const;
	real_t get_emission_ring_height() const;
	real_t get_emission_ring_radius() const;
	real_t get_emission_ring_inner_radius() const;
	real_t get_emission_ring_cone_angle() const;
	Ref<Curve> get_scale_curve_x() const;
	Ref<Curve> get_scale_curve_y() const;
	Ref<Curve> get_scale_curve_z() const;
	bool get_split_scale();

	void set_gravity(const Vector3& p_gravity);
	Vector3 get_gravity() const;

	PackedStringArray get_configuration_warnings() const override;

	void restart(bool p_keep_seed = false);

	void convert_from_particles(Node* p_particles);

	AABB capture_aabb() const;

	CPUParticles3D() = default;
	~CPUParticles3D();
};



// --- End scene/3d/cpu_particles_3d.h ---
#include "scene/3d/camera_3d.h"
// --- Inlined from scene/3d/gpu_particles_3d.h ---
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/3d/skin.h"

class Mesh;

class GPUParticles3D : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum DrawOrder
	{
		DRAW_ORDER_INDEX,
		DRAW_ORDER_LIFETIME,
		DRAW_ORDER_REVERSE_LIFETIME,
		DRAW_ORDER_VIEW_DEPTH,
	};

	enum TransformAlign
	{
		TRANSFORM_ALIGN_DISABLED = RSE::PARTICLES_TRANSFORM_ALIGN_DISABLED,
		TRANSFORM_ALIGN_Z_BILLBOARD = RSE::PARTICLES_TRANSFORM_ALIGN_Z_BILLBOARD,
		TRANSFORM_ALIGN_Y_TO_VELOCITY = RSE::PARTICLES_TRANSFORM_ALIGN_Y_TO_VELOCITY,
		TRANSFORM_ALIGN_Z_BILLBOARD_Y_TO_VELOCITY =
			RSE::PARTICLES_TRANSFORM_ALIGN_Z_BILLBOARD_Y_TO_VELOCITY,
		TRANSFORM_ALIGN_LOCAL_BILLBOARD = RSE::PARTICLES_TRANSFORM_ALIGN_LOCAL_BILLBOARD,
	};

	enum
	{
		MAX_DRAW_PASSES = 4
	};

private:
	RID particles;

	bool emitting = false;
	bool active = false;
	bool signal_canceled = false;
	bool one_shot = false;
	int amount = 0;
	float amount_ratio = 1.0;
	double lifetime = 0.0;
	double pre_process_time = 0.0;
	real_t explosiveness_ratio = 0.0;
	real_t randomness_ratio = 0.0;
	double speed_scale = 0.0;
	AABB visibility_aabb;
	bool local_coords = false;
	int fixed_fps = 0;
	bool fractional_delta = false;
	bool interpolate = true;
	NodePath sub_emitter;
	real_t collision_base_size = 0.01;
	uint32_t seed = 0;
	bool use_fixed_seed = false;

	bool trail_enabled = false;
	double trail_lifetime = 0.3;

	TransformAlign transform_align = TRANSFORM_ALIGN_DISABLED;
	RSE::ParticlesTransformAlignCustomSrc transform_align_channel_filter =
		RSE::ParticlesTransformAlignCustomSrc::PARTICLES_ALIGN_CHANNEL_FILTER_X;
	RSE::ParticlesTransformAlignAxis transform_align_axis =
		RSE::ParticlesTransformAlignAxis::PARTICLES_ALIGN_AXIS_Y;
	bool transform_align_use_velocity = true;

	Ref<Material> process_material;

	DrawOrder draw_order = DRAW_ORDER_INDEX;

	Vector<Ref<Mesh>> draw_passes;
	Ref<Skin> skin;

	double time = 0.0;
	double emission_time = 0.0;
	double active_time = 0.0;
	float interp_to_end_factor = 0;
	Vector3 previous_velocity;
	Vector3 previous_position;

	void _attach_sub_emitter();

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	AABB get_aabb() const;

	void set_emitting(bool p_emitting);
	void set_amount(int p_amount);
	void set_lifetime(double p_lifetime);
	void set_one_shot(bool p_one_shot);
	void set_pre_process_time(double p_time);
	void set_explosiveness_ratio(real_t p_ratio);
	void set_randomness_ratio(real_t p_ratio);
	void set_use_local_coordinates(bool p_enable);
	void set_process_material(const Ref<Material>& p_material);
	void set_speed_scale(double p_scale);
	void set_collision_base_size(real_t p_ratio);
	void set_trail_lifetime(double p_seconds);
	void set_interp_to_end(float p_interp);

	bool is_emitting() const;
	int get_amount() const;

	double get_lifetime() const;
	bool get_one_shot() const;
	double get_pre_process_time() const;
	real_t get_explosiveness_ratio() const;
	real_t get_randomness_ratio() const;
	AABB get_visibility_aabb() const;
	bool get_use_local_coordinates() const;
	Ref<Material> get_process_material() const;
	double get_speed_scale() const;
	real_t get_collision_base_size() const;
	bool is_trail_enabled() const;
	double get_trail_lifetime() const;
	float get_interp_to_end() const;

	void set_amount_ratio(float p_ratio);
	float get_amount_ratio() const;

	void set_fixed_fps(int p_count);
	int get_fixed_fps() const;

	void set_fractional_delta(bool p_enable);
	bool get_fractional_delta() const;

	void set_interpolate(bool p_enable);
	bool get_interpolate() const;

	void set_draw_order(DrawOrder p_order);
	DrawOrder get_draw_order() const;

	void set_draw_passes(int p_count);
	int get_draw_passes() const;

	void set_draw_pass_mesh(int p_pass, const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_draw_pass_mesh(int p_pass) const;

	PackedStringArray get_configuration_warnings() const override;

	NodePath get_sub_emitter() const;

	Ref<Skin> get_skin() const;

	void set_transform_align(TransformAlign p_align);
	TransformAlign get_transform_align() const;

	void set_transform_align_channel_filter(
		RSE::ParticlesTransformAlignCustomSrc p_align_channel_filter);
	RSE::ParticlesTransformAlignCustomSrc get_transform_align_channel_filter() const;

	void set_transform_align_axis(RSE::ParticlesTransformAlignAxis p_axis);
	RSE::ParticlesTransformAlignAxis get_transform_align_axis() const;

	void restart(bool p_keep_seed = false);

	void set_use_fixed_seed(bool p_use_fixed_seed);
	bool get_use_fixed_seed() const;

	void set_seed(uint32_t p_seed);
	uint32_t get_seed() const;
	void request_particles_process(
		real_t p_requested_process_time, real_t p_request_process_time_residual = 0.0f);

	enum EmitFlags
	{
		EMIT_FLAG_POSITION = RSE::PARTICLES_EMIT_FLAG_POSITION,
		EMIT_FLAG_ROTATION_SCALE = RSE::PARTICLES_EMIT_FLAG_ROTATION_SCALE,
		EMIT_FLAG_VELOCITY = RSE::PARTICLES_EMIT_FLAG_VELOCITY,
		EMIT_FLAG_COLOR = RSE::PARTICLES_EMIT_FLAG_COLOR,
		EMIT_FLAG_CUSTOM = RSE::PARTICLES_EMIT_FLAG_CUSTOM
	};

	void emit_particle(const Transform3D& p_transform, const Vector3& p_velocity,
		const Color& p_color, const Color& p_custom, uint32_t p_emit_flags);

	AABB capture_aabb() const;
	void convert_from_particles(Node* p_particles);

	GPUParticles3D();
	~GPUParticles3D();
};



// --- End scene/3d/gpu_particles_3d.h ---
#include "scene/main/viewport.h"
#include "scene/resources/material.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/mesh.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"

AABB CPUParticles3D::get_aabb() const { return AABB(); }

void CPUParticles3D::set_emitting(bool p_emitting)
{
	if (emitting == p_emitting) {
		return;
	}

	if (p_emitting && !use_fixed_seed && one_shot) {
		set_seed(Math::rand());
	}

	emitting = p_emitting;
	if (emitting) {
		_set_emitting();
	}
}

void CPUParticles3D::_set_emitting()
{
	active = true;
	set_process_internal(true);
	// first update before rendering to avoid one frame delay after emitting starts
	if (time == 0) {
		_update_internal();
	}
}

void CPUParticles3D::set_amount(int p_amount)
{
	ERR_FAIL_COND_MSG(p_amount < 1, "Amount of particles must be greater than 0.");

	particles.resize(p_amount);
	{
		Particle* w = particles.ptrw();

		for (int i = 0; i < p_amount; i++) {
			w[i].active = false;
			w[i].custom[3] = 1.0; // Make sure w component isn't garbage data and doesn't break
								  // shaders with CUSTOM.y/Custom.w
		}
	}

	particle_data.resize((12 + 4 + 4) * p_amount);
	Renderer::multimesh_set_visible_instances(multimesh, -1);
	Renderer::multimesh_allocate_data(
		multimesh, p_amount, RSE::MULTIMESH_TRANSFORM_3D, true, true);

	particle_order.resize(p_amount);
}

void CPUParticles3D::set_lifetime(double p_lifetime)
{
	ERR_FAIL_COND_MSG(p_lifetime <= 0, "Particles lifetime must be greater than 0.");
	lifetime = p_lifetime;
}

void CPUParticles3D::set_one_shot(bool p_one_shot) { one_shot = p_one_shot; }

void CPUParticles3D::set_pre_process_time(double p_time) { pre_process_time = p_time; }

void CPUParticles3D::set_explosiveness_ratio(real_t p_ratio) { explosiveness_ratio = p_ratio; }

void CPUParticles3D::set_randomness_ratio(real_t p_ratio) { randomness_ratio = p_ratio; }

void CPUParticles3D::set_lifetime_randomness(double p_random) { lifetime_randomness = p_random; }

void CPUParticles3D::set_use_local_coordinates(bool p_enable) { local_coords = p_enable; }

void CPUParticles3D::set_speed_scale(double p_scale) { speed_scale = p_scale; }

bool CPUParticles3D::is_emitting() const { return emitting; }

int CPUParticles3D::get_amount() const { return particles.size(); }

double CPUParticles3D::get_lifetime() const { return lifetime; }

bool CPUParticles3D::get_one_shot() const { return one_shot; }

double CPUParticles3D::get_pre_process_time() const { return pre_process_time; }

real_t CPUParticles3D::get_explosiveness_ratio() const { return explosiveness_ratio; }

real_t CPUParticles3D::get_randomness_ratio() const { return randomness_ratio; }

AABB CPUParticles3D::get_visibility_aabb() const { return visibility_aabb; }

double CPUParticles3D::get_lifetime_randomness() const { return lifetime_randomness; }

bool CPUParticles3D::get_use_local_coordinates() const { return local_coords; }

double CPUParticles3D::get_speed_scale() const { return speed_scale; }

void CPUParticles3D::set_draw_order(DrawOrder p_order)
{
	ERR_FAIL_INDEX(p_order, DRAW_ORDER_MAX);
	draw_order = p_order;
}

CPUParticles3D::DrawOrder CPUParticles3D::get_draw_order() const { return draw_order; }

Ref<Mesh> CPUParticles3D::get_mesh() const { return mesh; }

void CPUParticles3D::set_fixed_fps(int p_count) { fixed_fps = p_count; }

int CPUParticles3D::get_fixed_fps() const { return fixed_fps; }

void CPUParticles3D::set_fractional_delta(bool p_enable) { fractional_delta = p_enable; }

bool CPUParticles3D::get_fractional_delta() const { return fractional_delta; }

void CPUParticles3D::restart(bool p_keep_seed)
{
	time = 0;
	frame_remainder = 0;
	cycle = 0;
	emitting = false;

	{
		int pc = particles.size();
		Particle* w = particles.ptrw();

		for (int i = 0; i < pc; i++) {
			w[i].active = false;
		}
	}
	if (!p_keep_seed && !use_fixed_seed) {
		seed = Math::rand();
	}

	emitting = true;
	_set_emitting();
}

void CPUParticles3D::set_direction(Vector3 p_direction) { direction = p_direction; }

Vector3 CPUParticles3D::get_direction() const { return direction; }

void CPUParticles3D::set_spread(real_t p_spread) { spread = p_spread; }

real_t CPUParticles3D::get_spread() const { return spread; }

void CPUParticles3D::set_flatness(real_t p_flatness) { flatness = p_flatness; }

real_t CPUParticles3D::get_flatness() const { return flatness; }

real_t CPUParticles3D::get_param_min(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return parameters_min[p_param];
}

real_t CPUParticles3D::get_param_max(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return parameters_max[p_param];
}

static void _adjust_curve_range(const Ref<Curve>& p_curve, real_t p_min, real_t p_max)
{
	Ref<Curve> curve = p_curve;
	if (curve.is_null()) {
		return;
	}

	curve->ensure_default_setup(p_min, p_max);
}

Ref<Curve> CPUParticles3D::get_param_curve(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, Ref<Curve>());

	return curve_parameters[p_param];
}

void CPUParticles3D::set_color(const Color& p_color) { color = p_color; }

Color CPUParticles3D::get_color() const { return color; }

void CPUParticles3D::set_color_ramp(const Ref<Gradient>& p_ramp) { color_ramp = p_ramp; }

Ref<Gradient> CPUParticles3D::get_color_ramp() const { return color_ramp; }

void CPUParticles3D::set_color_initial_ramp(const Ref<Gradient>& p_ramp)
{
	color_initial_ramp = p_ramp;
}

Ref<Gradient> CPUParticles3D::get_color_initial_ramp() const { return color_initial_ramp; }

bool CPUParticles3D::get_particle_flag(ParticleFlags p_particle_flag) const
{
	ERR_FAIL_INDEX_V(p_particle_flag, PARTICLE_FLAG_MAX, false);
	return particle_flags[p_particle_flag];
}

void CPUParticles3D::set_emission_points(const Vector<Vector3>& p_points)
{
	emission_points = p_points;
}

void CPUParticles3D::set_emission_normals(const Vector<Vector3>& p_normals)
{
	emission_normals = p_normals;
}

void CPUParticles3D::set_emission_colors(const Vector<Color>& p_colors)
{
	emission_colors = p_colors;
}

void CPUParticles3D::set_scale_curve_x(Ref<Curve> p_scale_curve) { scale_curve_x = p_scale_curve; }

void CPUParticles3D::set_scale_curve_y(Ref<Curve> p_scale_curve) { scale_curve_y = p_scale_curve; }

void CPUParticles3D::set_scale_curve_z(Ref<Curve> p_scale_curve) { scale_curve_z = p_scale_curve; }

real_t CPUParticles3D::get_emission_sphere_radius() const { return emission_sphere_radius; }

Vector3 CPUParticles3D::get_emission_box_extents() const { return emission_box_extents; }

Vector<Vector3> CPUParticles3D::get_emission_points() const { return emission_points; }

Vector<Vector3> CPUParticles3D::get_emission_normals() const { return emission_normals; }

Vector<Color> CPUParticles3D::get_emission_colors() const { return emission_colors; }

Vector3 CPUParticles3D::get_emission_ring_axis() const { return emission_ring_axis; }

real_t CPUParticles3D::get_emission_ring_height() const { return emission_ring_height; }

real_t CPUParticles3D::get_emission_ring_radius() const { return emission_ring_radius; }

real_t CPUParticles3D::get_emission_ring_inner_radius() const { return emission_ring_inner_radius; }

real_t CPUParticles3D::get_emission_ring_cone_angle() const { return emission_ring_cone_angle; }

CPUParticles3D::EmissionShape CPUParticles3D::get_emission_shape() const { return emission_shape; }

void CPUParticles3D::set_gravity(const Vector3& p_gravity) { gravity = p_gravity; }

Vector3 CPUParticles3D::get_gravity() const { return gravity; }

Ref<Curve> CPUParticles3D::get_scale_curve_x() const { return scale_curve_x; }

Ref<Curve> CPUParticles3D::get_scale_curve_y() const { return scale_curve_y; }

Ref<Curve> CPUParticles3D::get_scale_curve_z() const { return scale_curve_z; }

bool CPUParticles3D::get_split_scale() { return split_scale; }

AABB CPUParticles3D::capture_aabb() const
{
	Renderer::multimesh_set_custom_aabb(multimesh, AABB());
	return Renderer::multimesh_get_aabb(multimesh);
}

bool CPUParticles3D::get_use_fixed_seed() const { return use_fixed_seed; }

void CPUParticles3D::set_seed(uint32_t p_seed) { seed = p_seed; }

uint32_t CPUParticles3D::get_seed() const { return seed; }

void CPUParticles3D::request_particles_process(
	real_t p_request_process_time, real_t p_request_process_time_residual)
{
	_request_process_time = p_request_process_time;
	_request_process_time_residual = p_request_process_time_residual;
	_update_internal();
}

static uint32_t idhash(uint32_t x)
{
	x = ((x >> uint32_t(16)) ^ x) * uint32_t(0x45d9f3b);
	x = ((x >> uint32_t(16)) ^ x) * uint32_t(0x45d9f3b);
	x = (x >> uint32_t(16)) ^ x;
	return x;
}

static real_t rand_from_seed(uint32_t& seed)
{
	int k;
	int s = int(seed);
	if (s == 0) {
		s = 305420679;
	}
	k = s / 127773;
	s = 16807 * (s - k * 127773) - 2836 * k;
	if (s < 0) {
		s += 2147483647;
	}
	seed = uint32_t(s);
	return (seed % uint32_t(65536)) / 65535.0;
}

void CPUParticles3D::_update_internal()
{
	if (particles.is_empty() || !is_visible_in_tree()) {
		_set_redraw(false);
		return;
	}

	double delta = get_process_delta_time();
	if (!active && !emitting) {
		set_process_internal(false);
		_set_redraw(false);

		// reset variables
		time = 0;
		frame_remainder = 0;
		cycle = 0;
		return;
	}
	_set_redraw(true);

	bool processed = false;

	{
		float todo = time == 0 ? pre_process_time : 0;
		todo = todo > _request_process_time ? todo : _request_process_time;
		todo = todo > _request_process_time_residual ? todo : _request_process_time_residual;

		if (todo > 0.0) {
			real_t frame_time;
			if (fixed_fps > 0) {
				frame_time = 1.0 / fixed_fps;
			}
			else {
				frame_time = 1.0 / 30.0;
			}

			float tmp_scale = speed_scale;
			// We need this otherwise the speed scale of the particle system influences the TODO.
			speed_scale = 1.0;
			if (time == 0) {
				todo = pre_process_time;
				while (todo > 0.0) {
					_particles_process(frame_time > todo ? todo : frame_time);
					todo -= frame_time;
				}
			}
			if (_request_process_time > 0.0) {
				todo = _request_process_time;
				emitting = true;
				while (todo > 0.0) {
					_particles_process(frame_time > todo ? todo : frame_time);
					todo -= frame_time;
				}
			}
			if (_request_process_time_residual > 0.0) {
				emitting = false;
				todo = _request_process_time_residual;
				while (todo > 0.0) {
					_particles_process(frame_time > todo ? todo : frame_time);
					todo -= frame_time;
				}
			}
			speed_scale = tmp_scale;
			processed = true;
		}
		_request_process_time = 0;
		_request_process_time_residual = 0;
	}

	double frame_time;
	if (fixed_fps > 0) {
		frame_time = 1.0 / fixed_fps;
	}
	else {
		frame_time = 1.0 / 30.0;
	}

	if (fixed_fps > 0) {
		double decr = frame_time;

		double ldelta = delta;
		if (ldelta > 0.1) { // avoid recursive stalls if fps goes below 10
			ldelta = 0.1;
		}
		else if (ldelta < 0.0) {
			ldelta = 0.0;
		}
		double todo = frame_remainder + ldelta;

		while (todo >= frame_time) {
			_particles_process(frame_time);
			processed = true;
			todo -= decr;
		}

		frame_remainder = todo;

	}
	else {
		_particles_process(delta);
		processed = true;
	}

	if (processed) {
		_update_particle_data_buffer();
	}
}

void CPUParticles3D::_update_particle_data_buffer()
{
	MutexLock lock(update_mutex);

	int pc = particles.size();

	int* ow;
	int* order = nullptr;

	float* w = particle_data.ptrw();
	const Particle* r = particles.ptr();
	float* ptr = w;

	if (draw_order != DRAW_ORDER_INDEX) {
		ow = particle_order.ptrw();
		order = ow;

		for (int i = 0; i < pc; i++) {
			order[i] = i;
		}
		if (draw_order == DRAW_ORDER_LIFETIME) {
			SortArray<int, SortLifetime> sorter;
			sorter.compare.particles = r;
			sorter.sort(order, pc);
		}
		else if (draw_order == DRAW_ORDER_VIEW_DEPTH) {
			ERR_FAIL_NULL(get_viewport());
			Camera3D* c = get_viewport()->get_camera_3d();
			if (c) {
				Vector3 dir = c->get_global_transform().basis.get_column(2); // far away to close

				if (local_coords) {
					// will look different from Particles in editor as this is based on the camera
					// in the scenetree and not the editor camera
					dir = inv_emission_transform.xform(dir).normalized();
				}
				else {
					dir = dir.normalized();
				}

				SortArray<int, SortAxis> sorter;
				sorter.compare.particles = r;
				sorter.compare.axis = dir;
				sorter.sort(order, pc);
			}
		}
	}

	for (int i = 0; i < pc; i++) {
		int idx = order ? order[i] : i;

		Transform3D t = r[idx].transform;

		if (!local_coords) {
			t = inv_emission_transform * t;
		}

		if (r[idx].active) {
			ptr[0] = t.basis.rows[0][0];
			ptr[1] = t.basis.rows[0][1];
			ptr[2] = t.basis.rows[0][2];
			ptr[3] = t.origin.x;
			ptr[4] = t.basis.rows[1][0];
			ptr[5] = t.basis.rows[1][1];
			ptr[6] = t.basis.rows[1][2];
			ptr[7] = t.origin.y;
			ptr[8] = t.basis.rows[2][0];
			ptr[9] = t.basis.rows[2][1];
			ptr[10] = t.basis.rows[2][2];
			ptr[11] = t.origin.z;
		}
		else {
			memset(ptr, 0, sizeof(float) * 12);
		}

		Color c = r[idx].color;

		ptr[12] = c.r;
		ptr[13] = c.g;
		ptr[14] = c.b;
		ptr[15] = c.a;

		ptr[16] = r[idx].custom[0];
		ptr[17] = r[idx].custom[1];
		ptr[18] = r[idx].custom[2];
		ptr[19] = r[idx].custom[3];

		ptr += 20;
	}

	can_update.set();
}

void CPUParticles3D::_update_render_thread()
{
	MutexLock lock(update_mutex);

	if (can_update.is_set()) {
		Renderer::multimesh_set_buffer(multimesh, particle_data);
		can_update.clear(); // wait for next time
	}
}

void CPUParticles3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void CPUParticles3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		set_process_internal(emitting);

		// first update before rendering to avoid one frame delay after emitting starts
		if (emitting && (time == 0)) {
			_update_internal();
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_set_redraw(false);
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		// first update before rendering to avoid one frame delay after emitting starts
		if (emitting && (time == 0)) {
			_update_internal();
		}
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		_update_internal();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		Transform3D global_transform = get_global_transform();
		if (unlikely(global_transform.basis.determinant() == 0)) {
			return;
		}
		inv_emission_transform = global_transform.affine_inverse();

		if (!local_coords) {
			int pc = particles.size();

			float* w = particle_data.ptrw();
			const Particle* r = particles.ptr();
			float* ptr = w;

			for (int i = 0; i < pc; i++) {
				Transform3D t = inv_emission_transform * r[i].transform;

				if (r[i].active) {
					ptr[0] = t.basis.rows[0][0];
					ptr[1] = t.basis.rows[0][1];
					ptr[2] = t.basis.rows[0][2];
					ptr[3] = t.origin.x;
					ptr[4] = t.basis.rows[1][0];
					ptr[5] = t.basis.rows[1][1];
					ptr[6] = t.basis.rows[1][2];
					ptr[7] = t.origin.y;
					ptr[8] = t.basis.rows[2][0];
					ptr[9] = t.basis.rows[2][1];
					ptr[10] = t.basis.rows[2][2];
					ptr[11] = t.origin.z;
				}
				else {
					memset(ptr, 0, sizeof(float) * 12);
				}

				ptr += 20;
			}

			can_update.set();
		}
	} break;
	}
}

CPUParticles3D::~CPUParticles3D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(multimesh);
}

void CPUParticles3D::_set_redraw(bool p_redraw) {}

void CPUParticles3D::_particles_process(double p_delta) {}

PackedStringArray CPUParticles3D::get_configuration_warnings() const { return PackedStringArray(); }

// ==========================================================================
// Merged from scene/3d/gpu_particles_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/os/os.h"


#include "scene/resources/material.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/mesh.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"

AABB GPUParticles3D::get_aabb() const { return AABB(); }

void GPUParticles3D::set_emitting(bool p_emitting)
{
	// Do not return even if `p_emitting == emitting` because `emitting` is just an approximation.
	if (p_emitting && p_emitting != emitting && !use_fixed_seed && one_shot) {
		set_seed(Math::rand());
	}
	if (p_emitting && one_shot) {
		if (!active && !emitting) {
			// Last cycle ended.
			active = true;
			time = 0;
			signal_canceled = false;
			emission_time = lifetime;
			active_time = lifetime * (2 - explosiveness_ratio);
		}
		else {
			signal_canceled = true;
		}
		set_process_internal(true);
	}
	else if (!p_emitting) {
		if (one_shot) {
			set_process_internal(true);
		}
		else {
			set_process_internal(false);
		}
	}
	else {
		set_process_internal(true);
	}

	emitting = p_emitting;
	Renderer::particles_set_emitting(particles, p_emitting);
}

void GPUParticles3D::set_amount(int p_amount)
{
	ERR_FAIL_COND_MSG(p_amount < 1, "Amount of particles cannot be smaller than 1.");
	amount = p_amount;
	Renderer::particles_set_amount(particles, amount);
}

void GPUParticles3D::set_lifetime(double p_lifetime)
{
	ERR_FAIL_COND_MSG(p_lifetime <= 0, "Particles lifetime must be greater than 0.");
	lifetime = p_lifetime;
	Renderer::particles_set_lifetime(particles, lifetime);
}

void GPUParticles3D::set_interp_to_end(float p_interp)
{
	interp_to_end_factor = CLAMP(p_interp, 0.0, 1.0);
	Renderer::particles_set_interp_to_end(particles, interp_to_end_factor);
}

void GPUParticles3D::set_one_shot(bool p_one_shot)
{
	one_shot = p_one_shot;
	Renderer::particles_set_one_shot(particles, one_shot);

	if (is_emitting()) {
		if (!one_shot) {
			Renderer::particles_restart(particles);
		}
	}
}

bool GPUParticles3D::get_use_fixed_seed() const { return use_fixed_seed; }

void GPUParticles3D::set_seed(uint32_t p_seed)
{
	seed = p_seed;
	Renderer::particles_set_seed(particles, p_seed);
}

uint32_t GPUParticles3D::get_seed() const { return seed; }

void GPUParticles3D::set_pre_process_time(double p_time)
{
	pre_process_time = p_time;
	Renderer::particles_set_pre_process_time(particles, pre_process_time);
}

void GPUParticles3D::set_explosiveness_ratio(real_t p_ratio)
{
	explosiveness_ratio = p_ratio;
	Renderer::particles_set_explosiveness_ratio(particles, explosiveness_ratio);
}

void GPUParticles3D::set_randomness_ratio(real_t p_ratio)
{
	randomness_ratio = p_ratio;
	Renderer::particles_set_randomness_ratio(particles, randomness_ratio);
}

void GPUParticles3D::set_use_local_coordinates(bool p_enable)
{
	local_coords = p_enable;
	Renderer::particles_set_use_local_coordinates(particles, local_coords);
}

void GPUParticles3D::set_speed_scale(double p_scale)
{
	speed_scale = p_scale;
	Renderer::particles_set_speed_scale(particles, p_scale);
}

void GPUParticles3D::set_collision_base_size(real_t p_size)
{
	collision_base_size = p_size;
	Renderer::particles_set_collision_base_size(particles, p_size);
}

bool GPUParticles3D::is_emitting() const { return emitting; }

int GPUParticles3D::get_amount() const { return amount; }

double GPUParticles3D::get_lifetime() const { return lifetime; }

float GPUParticles3D::get_interp_to_end() const { return interp_to_end_factor; }

bool GPUParticles3D::get_one_shot() const { return one_shot; }

double GPUParticles3D::get_pre_process_time() const { return pre_process_time; }

real_t GPUParticles3D::get_explosiveness_ratio() const { return explosiveness_ratio; }

real_t GPUParticles3D::get_randomness_ratio() const { return randomness_ratio; }

AABB GPUParticles3D::get_visibility_aabb() const { return visibility_aabb; }

bool GPUParticles3D::get_use_local_coordinates() const { return local_coords; }

Ref<Material> GPUParticles3D::get_process_material() const { return process_material; }

double GPUParticles3D::get_speed_scale() const { return speed_scale; }

real_t GPUParticles3D::get_collision_base_size() const { return collision_base_size; }

void GPUParticles3D::set_draw_order(DrawOrder p_order)
{
	draw_order = p_order;
	Renderer::particles_set_draw_order(particles, RSE::ParticlesDrawOrder(p_order));
}

void GPUParticles3D::set_trail_lifetime(double p_seconds)
{
	ERR_FAIL_COND(p_seconds < 0.01 - CMP_EPSILON);
	trail_lifetime = p_seconds;
	Renderer::particles_set_trails(particles, trail_enabled, trail_lifetime);
}

bool GPUParticles3D::is_trail_enabled() const { return trail_enabled; }

double GPUParticles3D::get_trail_lifetime() const { return trail_lifetime; }

GPUParticles3D::DrawOrder GPUParticles3D::get_draw_order() const { return draw_order; }

int GPUParticles3D::get_draw_passes() const { return draw_passes.size(); }

Ref<Mesh> GPUParticles3D::get_draw_pass_mesh(int p_pass) const
{
	ERR_FAIL_INDEX_V(p_pass, draw_passes.size(), Ref<Mesh>());

	return draw_passes[p_pass];
}

void GPUParticles3D::set_fixed_fps(int p_count)
{
	fixed_fps = p_count;
	Renderer::particles_set_fixed_fps(particles, p_count);
}

int GPUParticles3D::get_fixed_fps() const { return fixed_fps; }

void GPUParticles3D::set_fractional_delta(bool p_enable)
{
	fractional_delta = p_enable;
	Renderer::particles_set_fractional_delta(particles, p_enable);
}

bool GPUParticles3D::get_fractional_delta() const { return fractional_delta; }

void GPUParticles3D::set_interpolate(bool p_enable)
{
	interpolate = p_enable;
	Renderer::particles_set_interpolate(particles, p_enable);
}

bool GPUParticles3D::get_interpolate() const { return interpolate; }

void GPUParticles3D::restart(bool p_keep_seed)
{
	if (!p_keep_seed && !use_fixed_seed) {
		set_seed(Math::rand());
	}
	Renderer::particles_restart(particles);
	Renderer::particles_set_emitting(particles, true);

	emitting = true;
	active = true;
	signal_canceled = false;
	time = 0;
	emission_time = lifetime * (1 - explosiveness_ratio);
	active_time = lifetime * (2 - explosiveness_ratio);
	set_process_internal(true);
}

AABB GPUParticles3D::capture_aabb() const
{
	return Renderer::particles_get_current_aabb(particles);
}

void GPUParticles3D::request_particles_process(
	real_t p_requested_process_time, real_t p_request_process_time_residual)
{
	Renderer::particles_request_process_time(
		particles, p_requested_process_time, p_request_process_time_residual);
	// Setting emitting independently from set_emitting is important here
	// we assume to be in a controlled process situation.
	// setting RS emission to true is necessary to ensure particles will emit when
	// regular process time is > than zero but the particles are already in trailing mode.
	// this could also be done on the GScript/tool side.
	if (p_requested_process_time > 0.0) {
		emitting = true;
		Renderer::particles_set_emitting(particles, true);
	}
	if (p_request_process_time_residual > 0.0) {
		emitting = false;
	}
}

void GPUParticles3D::emit_particle(const Transform3D& p_transform, const Vector3& p_velocity,
	const Color& p_color, const Color& p_custom, uint32_t p_emit_flags)
{
	Renderer::particles_emit(
		particles, p_transform, p_velocity, p_color, p_custom, p_emit_flags);
}

NodePath GPUParticles3D::get_sub_emitter() const { return sub_emitter; }

Ref<Skin> GPUParticles3D::get_skin() const { return skin; }

GPUParticles3D::TransformAlign GPUParticles3D::get_transform_align() const
{
	return transform_align;
}

void GPUParticles3D::set_transform_align_channel_filter(
	RSE::ParticlesTransformAlignCustomSrc p_align_channel_filter)
{
	ERR_FAIL_INDEX(uint32_t(p_align_channel_filter),
		uint32_t(RSE::ParticlesTransformAlignCustomSrc::PARTICLES_ALIGN_CHANNEL_FILTER_MAX));
	transform_align_channel_filter = p_align_channel_filter;
	Renderer::particles_set_transform_align_channel_filter(
		particles, transform_align_channel_filter);
}

RSE::ParticlesTransformAlignCustomSrc GPUParticles3D::get_transform_align_channel_filter() const
{
	return transform_align_channel_filter;
}

void GPUParticles3D::set_transform_align_axis(RSE::ParticlesTransformAlignAxis p_axis)
{
	transform_align_axis = p_axis;
	Renderer::particles_set_transform_align_axis(particles, p_axis);
}

RSE::ParticlesTransformAlignAxis GPUParticles3D::get_transform_align_axis() const
{
	return transform_align_axis;
}

void GPUParticles3D::set_amount_ratio(float p_ratio)
{
	amount_ratio = p_ratio;
	Renderer::particles_set_amount_ratio(particles, p_ratio);
}

float GPUParticles3D::get_amount_ratio() const { return amount_ratio; }

GPUParticles3D::GPUParticles3D()
{
	particles = Renderer::particles_create();
	Renderer::particles_set_mode(particles, RSE::PARTICLES_MODE_3D);
	set_base(particles);
	one_shot = false; // Needed so that set_emitting doesn't access uninitialized values
	set_emitting(true);
	set_one_shot(false);
	set_seed(Math::rand());
	set_amount_ratio(1.0);
	set_amount(8);
	set_lifetime(1);
	set_fixed_fps(30);
	set_fractional_delta(true);
	set_interpolate(true);
	set_pre_process_time(0);
	set_explosiveness_ratio(0);
	set_randomness_ratio(0);
	set_trail_lifetime(0.3);
	set_use_local_coordinates(false);
	set_draw_passes(1);
	set_draw_order(DRAW_ORDER_INDEX);
	set_speed_scale(1);
	set_collision_base_size(collision_base_size);
	set_transform_align(TRANSFORM_ALIGN_DISABLED);
	set_transform_align_channel_filter(
		RSE::ParticlesTransformAlignCustomSrc::PARTICLES_ALIGN_CHANNEL_FILTER_X);
	set_transform_align_axis(RSE::ParticlesTransformAlignAxis::PARTICLES_ALIGN_AXIS_Y);
	set_use_fixed_seed(false);
}

void GPUParticles3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void GPUParticles3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

GPUParticles3D::~GPUParticles3D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(particles);
}

PackedStringArray GPUParticles3D::get_configuration_warnings() const { return PackedStringArray(); }

void GPUParticles3D::set_use_fixed_seed(bool) {}

void GPUParticles3D::set_transform_align(GPUParticles3D::TransformAlign) {}

void GPUParticles3D::set_draw_passes(int) {}

// ==========================================================================
// Merged from scene/3d/gpu_particles_collision_3d.cpp
// ==========================================================================
// --- Inlined from scene/3d/gpu_particles_collision_3d.h ---
#include "core/templates/local_vector.h"
#include "scene/3d/visual_instance_3d.h"

class Mesh;

class GPUParticlesCollision3D : public Node3D
{
public:
	enum ShapeType
	{
		SHAPE_SPHERE,
		SHAPE_BOX,
		SHAPE_SDF,
		SHAPE_HEIGHT_FIELD,
	};

	enum SDFResolution
	{
		SDF_RESOLUTION_16,
		SDF_RESOLUTION_32,
		SDF_RESOLUTION_64,
		SDF_RESOLUTION_128,
		SDF_RESOLUTION_256,
		SDF_RESOLUTION_512,
		SDF_RESOLUTION_MAX,
	};

	enum HeightFieldResolution
	{
		HEIGHTFIELD_RESOLUTION_256,
		HEIGHTFIELD_RESOLUTION_512,
		HEIGHTFIELD_RESOLUTION_1024,
		HEIGHTFIELD_RESOLUTION_2048,
		HEIGHTFIELD_RESOLUTION_4096,
		HEIGHTFIELD_RESOLUTION_8192,
		HEIGHTFIELD_RESOLUTION_MAX,
	};

	enum UpdateMode
	{
		UPDATE_MODE_WHEN_MOVED,
		UPDATE_MODE_ALWAYS,
	};

	typedef void (*BakeBeginFunc)(int);
	typedef void (*BakeStepFunc)(int, const String&);
	typedef void (*BakeEndFunc)();

	RenderInstance3D render_instance;

private:
	ShapeType shape_type = SHAPE_BOX;
	uint32_t cull_mask = 0xFFFFFFFF;
	RID collision;

	real_t radius = 1.0;
	Vector3 size = Vector3(2, 2, 2);

	// SDF parameters
	SDFResolution sdf_resolution = SDF_RESOLUTION_64;
	uint32_t bake_mask = 0xFFFFFFFF;
	Ref<Texture3D> texture;
	float thickness = 1.0f;

	// HeightField parameters
	uint32_t heightfield_mask = (1 << 20) - 1;
	HeightFieldResolution heightfield_resolution = HEIGHTFIELD_RESOLUTION_1024;
	bool follow_camera_mode = false;
	UpdateMode update_mode = UPDATE_MODE_WHEN_MOVED;

	void _apply_shape_type();

protected:
	_FORCE_INLINE_ RID _get_collision() const { return collision; }
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	static BakeBeginFunc bake_begin_function;
	static BakeStepFunc bake_step_function;
	static BakeEndFunc bake_end_function;

	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_shape_type(ShapeType p_type);
	ShapeType get_shape_type() const { return shape_type; }

	void set_cull_mask(uint32_t p_cull_mask);
	uint32_t get_cull_mask() const;

	void set_radius(real_t p_radius);
	real_t get_radius() const { return radius; }

	void set_size(const Vector3& p_size);
	Vector3 get_size() const { return size; }

	void set_thickness(float p_thickness);
	float get_thickness() const { return thickness; }

	SDFResolution get_sdf_resolution() const { return sdf_resolution; }
	uint32_t get_bake_mask() const { return bake_mask; }
	bool get_bake_mask_value(int p_layer_number) const;

	void set_texture(const Ref<Texture3D>& p_texture);
	Ref<Texture3D> get_texture() const { return texture; }

	Vector3i get_estimated_cell_size() const;

	HeightFieldResolution get_heightfield_resolution() const { return heightfield_resolution; }

	void set_update_mode(UpdateMode p_update_mode);
	UpdateMode get_update_mode() const { return update_mode; }

	void set_heightfield_mask(uint32_t p_heightfield_mask);
	uint32_t get_heightfield_mask() const { return heightfield_mask; }

	void set_heightfield_mask_value(int p_layer_number, bool p_value);
	bool get_heightfield_mask_value(int p_layer_number) const;

	void set_follow_camera_enabled(bool p_enabled);
	bool is_follow_camera_enabled() const { return follow_camera_mode; }

	AABB get_aabb() const;
	virtual PackedStringArray get_configuration_warnings() const override;

	GPUParticlesCollision3D(ShapeType p_type = SHAPE_BOX);
	~GPUParticlesCollision3D();
};

class GPUParticlesAttractor3D : public Node3D
{
public:
	enum AttractorType
	{
		ATTRACTOR_SPHERE,
		ATTRACTOR_BOX,
		ATTRACTOR_VECTOR_FIELD,
	};

	RenderInstance3D render_instance;

private:
	AttractorType attractor_type = ATTRACTOR_SPHERE;
	uint32_t cull_mask = 0xFFFFFFFF;
	RID collision;
	real_t strength = 1.0;
	real_t attenuation = 1.0;
	real_t directionality = 0.0;

	real_t radius = 1.0;
	Vector3 size = Vector3(2, 2, 2);
	Ref<Texture3D> texture;

	void _apply_attractor_type();

protected:
	_FORCE_INLINE_ RID _get_collision() const { return collision; }
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_attractor_type(AttractorType p_type);
	AttractorType get_attractor_type() const { return attractor_type; }

	void set_cull_mask(uint32_t p_cull_mask);
	uint32_t get_cull_mask() const;

	void set_strength(real_t p_strength);
	real_t get_strength() const;

	void set_attenuation(real_t p_attenuation);
	real_t get_attenuation() const;

	real_t get_directionality() const;

	void set_radius(real_t p_radius);
	real_t get_radius() const { return radius; }

	void set_size(const Vector3& p_size);
	Vector3 get_size() const { return size; }

	void set_texture(const Ref<Texture3D>& p_texture);
	Ref<Texture3D> get_texture() const { return texture; }

	AABB get_aabb() const;

	GPUParticlesAttractor3D(AttractorType p_type = ATTRACTOR_SPHERE);
	~GPUParticlesAttractor3D();
};

// --- End scene/3d/gpu_particles_collision_3d.h ---

#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/main/viewport.h"
#include "servers/rendering/renderer.h"

GPUParticlesCollision3D::BakeBeginFunc GPUParticlesCollision3D::bake_begin_function = nullptr;
GPUParticlesCollision3D::BakeStepFunc GPUParticlesCollision3D::bake_step_function = nullptr;
GPUParticlesCollision3D::BakeEndFunc GPUParticlesCollision3D::bake_end_function = nullptr;

void GPUParticlesCollision3D::_apply_shape_type()
{
	if (!collision.is_valid()) {
		return;
	}
	RSE::ParticlesCollisionType rs_type = RSE::PARTICLES_COLLISION_TYPE_BOX_COLLIDE;
	switch (shape_type) {
	case SHAPE_SPHERE:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_SPHERE_COLLIDE;
		break;
	case SHAPE_BOX:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_BOX_COLLIDE;
		break;
	case SHAPE_SDF:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_SDF_COLLIDE;
		break;
	case SHAPE_HEIGHT_FIELD:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_HEIGHTFIELD_COLLIDE;
		break;
	}
	Renderer::particles_collision_set_collision_type(collision, rs_type);
}

void GPUParticlesCollision3D::set_shape_type(ShapeType p_type)
{
	shape_type = p_type;
	_apply_shape_type();
	set_process_internal(shape_type == SHAPE_HEIGHT_FIELD &&
						 (follow_camera_mode || update_mode == UPDATE_MODE_ALWAYS));
}

void GPUParticlesCollision3D::set_cull_mask(uint32_t p_cull_mask)
{
	cull_mask = p_cull_mask;
	Renderer::particles_collision_set_cull_mask(collision, p_cull_mask);
}

uint32_t GPUParticlesCollision3D::get_cull_mask() const { return cull_mask; }

void GPUParticlesCollision3D::set_radius(real_t p_radius)
{
	radius = p_radius;
	Renderer::particles_collision_set_sphere_radius(collision, radius);
}

void GPUParticlesCollision3D::set_size(const Vector3& p_size)
{
	size = p_size;
	Renderer::particles_collision_set_box_extents(collision, size / 2);
}

void GPUParticlesCollision3D::set_thickness(float p_thickness) { thickness = p_thickness; }

bool GPUParticlesCollision3D::get_bake_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 20, false,
		vformat(
			"The render layer number (%d) must be between 1 and 20 (inclusive).", p_layer_number));
	return bake_mask & (1 << (p_layer_number - 1));
}

void GPUParticlesCollision3D::set_texture(const Ref<Texture3D>& p_texture)
{
	texture = p_texture;
	RID tex = texture.is_valid() ? texture->get_rid() : RID();
	Renderer::particles_collision_set_field_texture(collision, tex);
}

Vector3i GPUParticlesCollision3D::get_estimated_cell_size() const
{
	static const int subdivs[SDF_RESOLUTION_MAX] = {16, 32, 64, 128, 256, 512};
	int subdiv = subdivs[sdf_resolution];
	AABB aabb(-size / 2, size);
	float cell_size = aabb.get_longest_axis_size() / float(subdiv);
	Vector3i sdf_size = Vector3i(aabb.size / cell_size);
	return sdf_size.maxi(1);
}

void GPUParticlesCollision3D::set_update_mode(UpdateMode p_update_mode)
{
	update_mode = p_update_mode;
	set_process_internal(shape_type == SHAPE_HEIGHT_FIELD &&
						 (follow_camera_mode || update_mode == UPDATE_MODE_ALWAYS));
}

void GPUParticlesCollision3D::set_heightfield_mask(uint32_t p_heightfield_mask)
{
	heightfield_mask = p_heightfield_mask;
	Renderer::particles_collision_set_height_field_mask(collision, p_heightfield_mask);
}

void GPUParticlesCollision3D::set_heightfield_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 20, "Render layer number must be between 1 and 20 inclusive.");
	uint32_t mask = get_heightfield_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_heightfield_mask(mask);
}

bool GPUParticlesCollision3D::get_heightfield_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 20, false, "Render layer number must be between 1 and 20 inclusive.");
	return heightfield_mask & (1 << (p_layer_number - 1));
}

void GPUParticlesCollision3D::set_follow_camera_enabled(bool p_enabled)
{
	follow_camera_mode = p_enabled;
	set_process_internal(shape_type == SHAPE_HEIGHT_FIELD &&
						 (follow_camera_mode || update_mode == UPDATE_MODE_ALWAYS));
}

AABB GPUParticlesCollision3D::get_aabb() const
{
	if (shape_type == SHAPE_SPHERE) {
		return AABB(
			Vector3(-radius, -radius, -radius), Vector3(radius * 2, radius * 2, radius * 2));
	}
	return AABB(-size / 2, size);
}

PackedStringArray GPUParticlesCollision3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (shape_type == SHAPE_SDF && bake_mask == 0) {
		warnings.push_back(RTR("The Bake Mask has no bits enabled, which means baking will not "
							   "produce any collision for this GPUParticlesCollision3D.\nTo "
							   "resolve this, enable at least one bit in the Bake Mask property."));
	}

	return warnings;
}

void GPUParticlesCollision3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void GPUParticlesCollision3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	if (shape_type != SHAPE_HEIGHT_FIELD) {
		return;
	}

	switch (p_what) {
	case NOTIFICATION_INTERNAL_PROCESS: {
		if (update_mode == UPDATE_MODE_ALWAYS) {
			Renderer::particles_collision_height_field_update(collision);
		}

		if (follow_camera_mode && get_viewport()) {
			Camera3D* cam = get_viewport()->get_camera_3d();
			if (cam) {
				Transform3D xform = get_global_transform();
				Vector3 x_axis = xform.basis.get_column(Vector3::AXIS_X).normalized();
				Vector3 z_axis = xform.basis.get_column(Vector3::AXIS_Z).normalized();
				float x_len = xform.basis.get_scale().x;
				float z_len = xform.basis.get_scale().z;

				Vector3 cam_pos = cam->get_global_transform().origin;
				Transform3D new_xform = xform;

				while (x_axis.dot(cam_pos - new_xform.origin) > x_len) {
					new_xform.origin += x_axis * x_len;
				}
				while (x_axis.dot(cam_pos - new_xform.origin) < -x_len) {
					new_xform.origin -= x_axis * x_len;
				}

				while (z_axis.dot(cam_pos - new_xform.origin) > z_len) {
					new_xform.origin += z_axis * z_len;
				}
				while (z_axis.dot(cam_pos - new_xform.origin) < -z_len) {
					new_xform.origin -= z_axis * z_len;
				}

				if (new_xform != xform) {
					set_global_transform(new_xform);
					Renderer::particles_collision_height_field_update(collision);
				}
			}
		}
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		Renderer::particles_collision_height_field_update(collision);
	} break;
	}
}

GPUParticlesCollision3D::GPUParticlesCollision3D(ShapeType p_type) : shape_type(p_type)
{
	if (Renderer::data) {
		collision = Renderer::particles_collision_create();
		_apply_shape_type();
		set_base(collision);
	}
}

GPUParticlesCollision3D::~GPUParticlesCollision3D()
{
	if (collision.is_valid() && Renderer::data) {
		Renderer::free_rid(collision);
	}
}

void GPUParticlesAttractor3D::_apply_attractor_type()
{
	if (!collision.is_valid()) {
		return;
	}
	RSE::ParticlesCollisionType rs_type = RSE::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT;
	switch (attractor_type) {
	case ATTRACTOR_SPHERE:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT;
		break;
	case ATTRACTOR_BOX:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_BOX_ATTRACT;
		break;
	case ATTRACTOR_VECTOR_FIELD:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_VECTOR_FIELD_ATTRACT;
		break;
	}
	Renderer::particles_collision_set_collision_type(collision, rs_type);
}

void GPUParticlesAttractor3D::set_attractor_type(AttractorType p_type)
{
	attractor_type = p_type;
	_apply_attractor_type();
}

void GPUParticlesAttractor3D::set_cull_mask(uint32_t p_cull_mask)
{
	cull_mask = p_cull_mask;
	Renderer::particles_collision_set_cull_mask(collision, p_cull_mask);
}

uint32_t GPUParticlesAttractor3D::get_cull_mask() const { return cull_mask; }

void GPUParticlesAttractor3D::set_strength(real_t p_strength)
{
	strength = p_strength;
	Renderer::particles_collision_set_attractor_strength(collision, p_strength);
}

real_t GPUParticlesAttractor3D::get_strength() const { return strength; }

void GPUParticlesAttractor3D::set_attenuation(real_t p_attenuation)
{
	attenuation = p_attenuation;
	Renderer::particles_collision_set_attractor_attenuation(collision, p_attenuation);
}

real_t GPUParticlesAttractor3D::get_attenuation() const { return attenuation; }

real_t GPUParticlesAttractor3D::get_directionality() const { return directionality; }

void GPUParticlesAttractor3D::set_radius(real_t p_radius)
{
	radius = p_radius;
	Renderer::particles_collision_set_sphere_radius(collision, radius);
}

void GPUParticlesAttractor3D::set_size(const Vector3& p_size)
{
	size = p_size;
	Renderer::particles_collision_set_box_extents(collision, size / 2);
}

void GPUParticlesAttractor3D::set_texture(const Ref<Texture3D>& p_texture)
{
	texture = p_texture;
	RID tex = texture.is_valid() ? texture->get_rid() : RID();
	Renderer::particles_collision_set_field_texture(collision, tex);
}

AABB GPUParticlesAttractor3D::get_aabb() const
{
	if (attractor_type == ATTRACTOR_SPHERE) {
		return AABB(
			Vector3(-radius, -radius, -radius), Vector3(radius * 2, radius * 2, radius * 2));
	}
	return AABB(-size / 2, size);
}

void GPUParticlesAttractor3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void GPUParticlesAttractor3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

GPUParticlesAttractor3D::GPUParticlesAttractor3D(AttractorType p_type) : attractor_type(p_type)
{
	if (Renderer::data) {
		collision = Renderer::particles_collision_create();
		_apply_attractor_type();
		set_base(collision);
	}
}

GPUParticlesAttractor3D::~GPUParticlesAttractor3D()
{
	if (collision.is_valid() && Renderer::data) {
		Renderer::free_rid(collision);
	}
}

// ============================================================================
// Merged from scene/3d/light_3d.cpp
// ============================================================================

#include "scene/3d/visual_instance_3d.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"

void Light3D::_create_light_rid()
{
	if (!Renderer::data) {
		return;
	}
	switch (type) {
	case RSE::LIGHT_DIRECTIONAL:
		light = Renderer::directional_light_create();
		break;
	case RSE::LIGHT_OMNI:
		light = Renderer::omni_light_create();
		Renderer::light_omni_set_shadow_mode(light, RSE::LightOmniShadowMode(omni_shadow_mode));
		break;
	case RSE::LIGHT_SPOT:
		light = Renderer::spot_light_create();
		break;
	case RSE::LIGHT_AREA:
		light = Renderer::area_light_create();
		break;
	}
	set_base(light);
}

void Light3D::_free_light_rid()
{
	if (light.is_valid() && Renderer::data) {
		set_base(RID());
		Renderer::free_rid(light);
		light = RID();
	}
}

void Light3D::set_light_type(RSE::LightType p_type)
{
	if (type == p_type) {
		return;
	}
	_free_light_rid();
	type = p_type;
	_create_light_rid();
}

void Light3D::set_param(Param p_param, real_t p_value)
{
	ERR_FAIL_INDEX(p_param, PARAM_MAX);
	param[p_param] = p_value;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_param(light, RSE::LightParam(p_param), p_value);
	}
}

real_t Light3D::get_param(Param p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);
	return param[p_param];
}

void Light3D::set_shadow(bool p_enable)
{
	shadow = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_shadow(light, p_enable);
	}
}

bool Light3D::has_shadow() const { return shadow; }

void Light3D::set_negative(bool p_enable)
{
	negative = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_negative(light, p_enable);
	}
}

bool Light3D::is_negative() const { return negative; }

void Light3D::set_enable_distance_fade(bool p_enable)
{
	distance_fade_enabled = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

bool Light3D::is_distance_fade_enabled() const { return distance_fade_enabled; }

void Light3D::set_distance_fade_begin(real_t p_distance)
{
	distance_fade_begin = p_distance;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

real_t Light3D::get_distance_fade_begin() const { return distance_fade_begin; }

void Light3D::set_distance_fade_shadow(real_t p_distance)
{
	distance_fade_shadow = p_distance;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

real_t Light3D::get_distance_fade_shadow() const { return distance_fade_shadow; }

void Light3D::set_distance_fade_length(real_t p_length)
{
	distance_fade_length = p_length;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

real_t Light3D::get_distance_fade_length() const { return distance_fade_length; }

void Light3D::set_cull_mask(uint32_t p_cull_mask)
{
	cull_mask = p_cull_mask;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_cull_mask(light, p_cull_mask);
	}
}

uint32_t Light3D::get_cull_mask() const { return cull_mask; }

void Light3D::set_color(const Color& p_color)
{
	color = p_color;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_color(light, color * correlated_color);
	}
}

Color Light3D::get_color() const { return color; }

void Light3D::set_shadow_reverse_cull_face(bool p_enable)
{
	reverse_cull = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_reverse_cull_face_mode(light, reverse_cull);
	}
}

bool Light3D::get_shadow_reverse_cull_face() const { return reverse_cull; }

void Light3D::set_shadow_caster_mask(uint32_t p_caster_mask)
{
	shadow_caster_mask = p_caster_mask;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_shadow_caster_mask(light, shadow_caster_mask);
	}
}

uint32_t Light3D::get_shadow_caster_mask() const { return shadow_caster_mask; }

AABB Light3D::get_aabb() const { return AABB(); }

void Light3D::set_bake_mode(BakeMode p_mode)
{
	bake_mode = p_mode;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_bake_mode(light, RSE::LightBakeMode(p_mode));
	}
}

Light3D::BakeMode Light3D::get_bake_mode() const { return bake_mode; }

void Light3D::set_projector(const Ref<Texture2D>& p_texture)
{
	projector = p_texture;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_projector(light, projector.is_valid() ? projector->get_rid() : RID());
	}
}

Ref<Texture2D> Light3D::get_projector() const { return projector; }

void Light3D::owner_changed_notify()
{
	_update_visibility();
}

Color _color_from_temperature(float p_temperature)
{
	float T2 = p_temperature * p_temperature;
	float u = (0.860117757f + 1.54118254e-4f * p_temperature + 1.28641212e-7f * T2) /
			  (1.0f + 8.42420235e-4f * p_temperature + 7.08145163e-7f * T2);
	float v = (0.317398726f + 4.22806245e-5f * p_temperature + 4.20481691e-8f * T2) /
			  (1.0f - 2.89741816e-5f * p_temperature + 1.61456053e-7f * T2);

	float d = 1.0f / (2.0f * u - 8.0f * v + 4.0f);
	float x = 3.0f * u * d;
	float y = 2.0f * v * d;

	const float a = 1.0 / MAX(y, 1e-5f);
	Vector3 xyz = Vector3(x * a, 1.0, (1.0f - x - y) * a);

	Vector3 linear = Vector3(3.2404542f * xyz.x - 1.5371385f * xyz.y - 0.4985314f * xyz.z,
		-0.9692660f * xyz.x + 1.8760108f * xyz.y + 0.0415560f * xyz.z,
		0.0556434f * xyz.x - 0.2040259f * xyz.y + 1.0572252f * xyz.z);
	linear /= MAX(1e-5f, linear[linear.max_axis_index()]);
	return Color(linear.x, linear.y, linear.z).clamp().linear_to_srgb();
}

Color Light3D::get_correlated_color() const { return correlated_color; }

float Light3D::get_temperature() const { return temperature; }

void Light3D::_update_visibility()
{
	if (!is_inside_tree()) {
		return;
	}

	bool editor_ok = true;

#ifdef TOOLS_ENABLED
	if (editor_only) {
		if (!Engine::is_editor_hint()) {
			editor_ok = false;
		}
		else {
			editor_ok = (SceneTree::get_edited_scene_root() &&
						 (this == SceneTree::get_edited_scene_root() ||
							 get_owner() == SceneTree::get_edited_scene_root()));
		}
	}
#else
	if (editor_only) {
		editor_ok = false;
	}
#endif

	if (get_instance().is_valid() && Renderer::data) {
		Renderer::instance_set_visible(get_instance(), is_visible_in_tree() && editor_ok);
	}
}

void Light3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void Light3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED:
	case NOTIFICATION_ENTER_TREE: {
		_update_visibility();
	} break;
	}
}

void Light3D::set_editor_only(bool p_editor_only)
{
	editor_only = p_editor_only;
	_update_visibility();
}

bool Light3D::is_editor_only() const { return editor_only; }

void Light3D::set_directional_shadow_mode(DirectionalShadowMode p_mode)
{
	directional_shadow_mode = p_mode;
	if (type == RSE::LIGHT_DIRECTIONAL && light.is_valid() && Renderer::data) {
		Renderer::light_directional_set_shadow_mode(
			light, RSE::LightDirectionalShadowMode(p_mode));
	}
}

void Light3D::set_blend_splits(bool p_enable)
{
	blend_splits = p_enable;
	if (type == RSE::LIGHT_DIRECTIONAL && light.is_valid() && Renderer::data) {
		Renderer::light_directional_set_blend_splits(light, p_enable);
	}
}

void Light3D::set_sky_mode(SkyMode p_mode)
{
	sky_mode = p_mode;
	if (type == RSE::LIGHT_DIRECTIONAL && light.is_valid() && Renderer::data) {
		Renderer::light_directional_set_sky_mode(light, RSE::LightDirectionalSkyMode(p_mode));
	}
}

void Light3D::set_omni_shadow_mode(OmniShadowMode p_mode)
{
	omni_shadow_mode = p_mode;
	if (type == RSE::LIGHT_OMNI && light.is_valid() && Renderer::data) {
		Renderer::light_omni_set_shadow_mode(light, RSE::LightOmniShadowMode(p_mode));
	}
}

void Light3D::set_area_size(const Vector2& p_size)
{
	area_size = p_size;
	if (type == RSE::LIGHT_AREA && light.is_valid() && Renderer::data) {
		Renderer::light_area_set_size(light, area_size);
	}
}

void Light3D::set_area_texture(const Ref<Texture2D>& p_texture)
{
	area_texture = p_texture;
	if (type == RSE::LIGHT_AREA && light.is_valid() && Renderer::data) {
		Renderer::light_area_set_texture(
			light, area_texture.is_valid() ? area_texture->get_rid() : RID());
	}
}

void Light3D::set_area_normalize_energy(bool p_enabled)
{
	area_normalize_energy = p_enabled;
	if (type == RSE::LIGHT_AREA && light.is_valid() && Renderer::data) {
		Renderer::light_area_set_normalize_energy(light, p_enabled);
	}
}

PackedStringArray Light3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (!get_scale().is_equal_approx(Vector3(1, 1, 1))) {
		warnings.push_back(RTR("A light's scale does not affect the visual size of the light."));
	}

	switch (type) {
	case RSE::LIGHT_OMNI: {
		if (!has_shadow() && get_projector().is_valid()) {
			warnings.push_back(RTR("Projector texture only works with shadows active."));
		}
		if (get_projector().is_valid() &&
			(OS::get_current_rendering_method() == "gl_compatibility" ||
				OS::get_current_rendering_method() == "dummy")) {
			warnings.push_back(
				RTR("Projector textures are not supported when using the Compatibility renderer."));
		}
	} break;

	case RSE::LIGHT_SPOT: {
		if (has_shadow() && get_param(PARAM_SPOT_ANGLE) >= 90.0) {
			warnings.push_back(
				RTR("A SpotLight3D with an angle wider than 90 degrees cannot cast shadows."));
		}
		if (!has_shadow() && get_projector().is_valid()) {
			warnings.push_back(RTR("Projector texture only works with shadows active."));
		}
		if (get_projector().is_valid() &&
			(OS::get_current_rendering_method() == "gl_compatibility" ||
				OS::get_current_rendering_method() == "dummy")) {
			warnings.push_back(
				RTR("Projector textures are not supported when using the Compatibility renderer."));
		}
	} break;

	case RSE::LIGHT_AREA: {
		if (get_projector().is_valid()) {
			warnings.push_back(RTR("Projector texture is not supported for area lights. Use the "
								   "area_texture field instead."));
		}
		if (area_texture.is_valid() && OS::get_current_rendering_method() == "gl_compatibility") {
			warnings.push_back(RTR("Rendering textured area lights is not implemented in the "
								   "Compatibility rendering mode."));
		}
		if (has_shadow() && OS::get_current_rendering_method() == "gl_compatibility") {
			warnings.push_back(RTR(
				"Rendering area light shadows does not work in the Compatibility rendering mode."));
		}
	} break;

	default:
		break;
	}

	return warnings;
}

Light3D::Light3D(RSE::LightType p_type) : type(p_type)
{
	_create_light_rid();
}

Light3D::~Light3D()
{
	_free_light_rid();
}

// ==========================================================================
// Merged from scene/3d/voxelizer.cpp
// ==========================================================================

#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "core/math/geometry_3d.h"
#include "scene/resources/texture.h"
// --- voxelizer.h ---
#include "scene/resources/multimesh.h"

class Image;

class Voxelizer {
public:
	enum BakeResult {
		BAKE_RESULT_OK,
		BAKE_RESULT_INVALID_PARAMETER,
		BAKE_RESULT_CANCELLED,
	};

	typedef bool (*BakeStepFunc)(int, int);

private:
	enum : uint32_t {
		CHILD_EMPTY = 0xFFFFFFFF
	};

	struct Cell {
		uint32_t children[8];
		float albedo[3] = {}; //albedo in RGB24
		float emission[3] = {}; //accumulated light in 16:16 fixed point (needs to be integer for moving lights fast)
		float normal[3] = {};
		float alpha = 0.0; //used for upsampling
		uint16_t x = 0;
		uint16_t y = 0;
		uint16_t z = 0;
		uint16_t level = 0;

		Cell() {
			for (int i = 0; i < 8; i++) {
				children[i] = CHILD_EMPTY;
			}
		}
	};

	Vector<Cell> bake_cells;
	int cell_subdiv = 0;

	struct CellSort {
		union {
			uint64_t key = 0;
			struct {
				uint64_t z : 16;
				uint64_t y : 16;
				uint64_t x : 16;
				uint64_t level : 16;
			};
		};

		int32_t index = 0;

		_FORCE_INLINE_ bool operator<(const CellSort &p_cell_sort) const {
			return key < p_cell_sort.key;
		}
	};

	struct MaterialCache {
		//128x128 textures
		Vector<Color> albedo;
		Vector<Color> emission;
	};

	HashMap<Ref<Material>, MaterialCache> material_cache;
	float exposure_normalization = 1.0;
	AABB original_bounds;
	AABB po2_bounds;
	int axis_cell_size[3] = {};

	Transform3D to_cell_space;

	int color_scan_cell_width = 4;
	int bake_texture_size = 128;
	float cell_size = 0.0;

	int max_original_cells = 0;
	int leaf_voxel_count = 0;

	Vector<Color> _get_bake_texture(Ref<Image> p_image, const Color &p_color_mul, const Color &p_color_add);
	MaterialCache _get_material_cache(Ref<Material> p_material);

	void _plot_face(int p_idx, int p_level, int p_x, int p_y, int p_z, const Vector3 *p_vtx, const Vector3 *p_normal, const Vector2 *p_uv, const MaterialCache &p_material, const AABB &p_aabb);
	void _fixup_plot(int p_idx, int p_level);
	void _debug_mesh(int p_idx, int p_level, const AABB &p_aabb, Ref<MultiMesh> &p_multimesh, int &idx);

	bool sorted = false;
	void _sort();

public:
	void begin_bake(int p_subdiv, const AABB &p_bounds, float p_exposure_normalization);
	int get_bake_steps(Ref<Mesh> &p_mesh) const;
	BakeResult plot_mesh(const Transform3D &p_xform, Ref<Mesh> &p_mesh, const Vector<Ref<Material>> &p_materials, const Ref<Material> &p_override_material, BakeStepFunc p_bake_step_function);
	void end_bake();

	int get_voxel_gi_octree_depth() const;
	Vector3i get_voxel_gi_octree_size() const;
	int get_voxel_gi_cell_count() const;
	Vector<uint8_t> get_voxel_gi_octree_cells() const;
	Vector<uint8_t> get_voxel_gi_data_cells() const;
	Vector<int> get_voxel_gi_level_cell_count() const;
	BakeResult get_sdf_3d_image(Vector<uint8_t> &r_image, BakeStepFunc p_bake_step_function) const;

	Ref<MultiMesh> create_debug_multimesh();

	Transform3D get_to_cell_space_xform() const;
	Voxelizer();
};

static _FORCE_INLINE_ void get_uv_and_normal(const Vector3& p_pos, const Vector3* p_vtx,
	const Vector2* p_uv, const Vector3* p_normal, Vector2& r_uv, Vector3& r_normal)
{
	if (p_pos.is_equal_approx(p_vtx[0])) {
		r_uv = p_uv[0];
		r_normal = p_normal[0];
		return;
	}
	if (p_pos.is_equal_approx(p_vtx[1])) {
		r_uv = p_uv[1];
		r_normal = p_normal[1];
		return;
	}
	if (p_pos.is_equal_approx(p_vtx[2])) {
		r_uv = p_uv[2];
		r_normal = p_normal[2];
		return;
	}

	Vector3 v0 = p_vtx[1] - p_vtx[0];
	Vector3 v1 = p_vtx[2] - p_vtx[0];
	Vector3 v2 = p_pos - p_vtx[0];

	real_t d00 = v0.dot(v0);
	real_t d01 = v0.dot(v1);
	real_t d11 = v1.dot(v1);
	real_t d20 = v2.dot(v0);
	real_t d21 = v2.dot(v1);
	real_t denom = (d00 * d11 - d01 * d01);
	if (denom == 0) {
		r_uv = p_uv[0];
		r_normal = p_normal[0];
		return;
	}
	real_t v = (d11 * d20 - d01 * d21) / denom;
	real_t w = (d00 * d21 - d01 * d20) / denom;
	real_t u = 1.0f - v - w;

	r_uv = p_uv[0] * u + p_uv[1] * v + p_uv[2] * w;
	r_normal = (p_normal[0] * u + p_normal[1] * v + p_normal[2] * w).normalized();
}

void Voxelizer::_plot_face(int p_idx, int p_level, int p_x, int p_y, int p_z, const Vector3* p_vtx,
	const Vector3* p_normal, const Vector2* p_uv, const MaterialCache& p_material,
	const AABB& p_aabb)
{
	if (p_level == cell_subdiv) {
		// plot the face by guessing its albedo and emission value

		// find best axis to map to, for scanning values
		int closest_axis = 0;
		real_t closest_dot = 0;

		Plane plane = Plane(p_vtx[0], p_vtx[1], p_vtx[2]);
		Vector3 normal = plane.normal;

		for (int i = 0; i < 3; i++) {
			Vector3 axis;
			axis[i] = 1.0;
			real_t dot = Math::abs(normal.dot(axis));
			if (i == 0 || dot > closest_dot) {
				closest_axis = i;
				closest_dot = dot;
			}
		}

		Vector3 axis;
		axis[closest_axis] = 1.0;
		Vector3 t1;
		t1[(closest_axis + 1) % 3] = 1.0;
		Vector3 t2;
		t2[(closest_axis + 2) % 3] = 1.0;

		t1 *= p_aabb.size[(closest_axis + 1) % 3] / real_t(color_scan_cell_width);
		t2 *= p_aabb.size[(closest_axis + 2) % 3] / real_t(color_scan_cell_width);

		Color albedo_accum;
		Color emission_accum;
		Vector3 normal_accum;

		float alpha = 0.0;

		// map to a grid average in the best axis for this face
		for (int i = 0; i < color_scan_cell_width; i++) {
			Vector3 ofs_i = real_t(i) * t1;

			for (int j = 0; j < color_scan_cell_width; j++) {
				Vector3 ofs_j = real_t(j) * t2;

				Vector3 from = p_aabb.position + ofs_i + ofs_j;
				Vector3 to = from + t1 + t2 + axis * p_aabb.size[closest_axis];
				Vector3 half = (to - from) * 0.5;

				// is in this cell?
				if (!Geometry3D::triangle_box_overlap(from + half, half, p_vtx)) {
					continue; // face does not span this cell
				}

				// go from -size to +size*2 to avoid skipping collisions
				Vector3 ray_from = from + (t1 + t2) * 0.5 - axis * p_aabb.size[closest_axis];
				Vector3 ray_to = ray_from + axis * p_aabb.size[closest_axis] * 2;

				if (normal.dot(ray_from - ray_to) < 0) {
					SWAP(ray_from, ray_to);
				}

				Vector3 intersection;

				if (!plane.intersects_segment(ray_from, ray_to, &intersection)) {
					if (Math::abs(plane.distance_to(ray_from)) <
						Math::abs(plane.distance_to(ray_to))) {
						intersection = plane.project(ray_from);
					}
					else {
						intersection = plane.project(ray_to);
					}
				}

				intersection =
					Face3(p_vtx[0], p_vtx[1], p_vtx[2]).get_closest_point_to(intersection);

				Vector2 uv;
				Vector3 lnormal;
				get_uv_and_normal(intersection, p_vtx, p_uv, p_normal, uv, lnormal);
				if (lnormal == Vector3()) { // just in case normal is not provided
					lnormal = normal;
				}

				int uv_x = CLAMP(int(Math::fposmod(uv.x, (real_t)1.0) * bake_texture_size), 0,
					bake_texture_size - 1);
				int uv_y = CLAMP(int(Math::fposmod(uv.y, (real_t)1.0) * bake_texture_size), 0,
					bake_texture_size - 1);

				int ofs = uv_y * bake_texture_size + uv_x;
				albedo_accum.r += p_material.albedo[ofs].r;
				albedo_accum.g += p_material.albedo[ofs].g;
				albedo_accum.b += p_material.albedo[ofs].b;
				albedo_accum.a += p_material.albedo[ofs].a;

				emission_accum.r += p_material.emission[ofs].r;
				emission_accum.g += p_material.emission[ofs].g;
				emission_accum.b += p_material.emission[ofs].b;

				normal_accum += lnormal;

				alpha += 1.0;
			}
		}

		if (alpha == 0) {
			// could not in any way get texture information.. so use closest point to center

			Face3 f(p_vtx[0], p_vtx[1], p_vtx[2]);
			Vector3 inters = f.get_closest_point_to(p_aabb.get_center());

			Vector3 lnormal;
			Vector2 uv;
			get_uv_and_normal(inters, p_vtx, p_uv, p_normal, uv, normal);
			if (lnormal == Vector3()) { // just in case normal is not provided
				lnormal = normal;
			}

			int uv_x = CLAMP(
				Math::fposmod(uv.x, (real_t)1.0) * bake_texture_size, 0, bake_texture_size - 1);
			int uv_y = CLAMP(
				Math::fposmod(uv.y, (real_t)1.0) * bake_texture_size, 0, bake_texture_size - 1);

			int ofs = uv_y * bake_texture_size + uv_x;

			alpha = 1.0 / (color_scan_cell_width * color_scan_cell_width);

			albedo_accum.r = p_material.albedo[ofs].r * alpha;
			albedo_accum.g = p_material.albedo[ofs].g * alpha;
			albedo_accum.b = p_material.albedo[ofs].b * alpha;
			albedo_accum.a = p_material.albedo[ofs].a * alpha;

			emission_accum.r = p_material.emission[ofs].r * alpha;
			emission_accum.g = p_material.emission[ofs].g * alpha;
			emission_accum.b = p_material.emission[ofs].b * alpha;

			normal_accum = lnormal * alpha;

		}
		else {
			float accdiv = 1.0 / (color_scan_cell_width * color_scan_cell_width);
			alpha *= accdiv;

			albedo_accum.r *= accdiv;
			albedo_accum.g *= accdiv;
			albedo_accum.b *= accdiv;
			albedo_accum.a *= accdiv;

			emission_accum.r *= accdiv;
			emission_accum.g *= accdiv;
			emission_accum.b *= accdiv;

			normal_accum *= accdiv;
		}

		// put this temporarily here, corrected in a later step
		bake_cells.write[p_idx].albedo[0] += albedo_accum.r;
		bake_cells.write[p_idx].albedo[1] += albedo_accum.g;
		bake_cells.write[p_idx].albedo[2] += albedo_accum.b;
		bake_cells.write[p_idx].emission[0] += emission_accum.r;
		bake_cells.write[p_idx].emission[1] += emission_accum.g;
		bake_cells.write[p_idx].emission[2] += emission_accum.b;
		bake_cells.write[p_idx].normal[0] += normal_accum.x;
		bake_cells.write[p_idx].normal[1] += normal_accum.y;
		bake_cells.write[p_idx].normal[2] += normal_accum.z;
		bake_cells.write[p_idx].alpha += alpha;

	}
	else {
		// go down

		int half = (1 << cell_subdiv) >> (p_level + 1);
		for (int i = 0; i < 8; i++) {
			AABB aabb = p_aabb;
			aabb.size *= 0.5;

			int nx = p_x;
			int ny = p_y;
			int nz = p_z;

			if (i & 1) {
				aabb.position.x += aabb.size.x;
				nx += half;
			}
			if (i & 2) {
				aabb.position.y += aabb.size.y;
				ny += half;
			}
			if (i & 4) {
				aabb.position.z += aabb.size.z;
				nz += half;
			}
			// make sure to not plot beyond limits
			if (nx < 0 || nx >= axis_cell_size[0] || ny < 0 || ny >= axis_cell_size[1] || nz < 0 ||
				nz >= axis_cell_size[2]) {
				continue;
			}

			{
				AABB test_aabb = aabb;
				// test_aabb.grow_by(test_aabb.get_longest_axis_size()*0.05); //grow a bit to avoid
				// numerical error in real-time
				Vector3 qsize = test_aabb.size * 0.5; // quarter size, for fast aabb test

				if (!Geometry3D::triangle_box_overlap(test_aabb.position + qsize, qsize, p_vtx)) {
					// if (!Face3(p_vtx[0],p_vtx[1],p_vtx[2]).intersects_aabb2(aabb)) {
					// does not fit in child, go on
					continue;
				}
			}

			if (bake_cells[p_idx].children[i] == CHILD_EMPTY) {
				// sub cell must be created

				uint32_t child_idx = bake_cells.size();
				bake_cells.write[p_idx].children[i] = child_idx;
				bake_cells.resize(bake_cells.size() + 1);
				bake_cells.write[child_idx].level = p_level + 1;
				bake_cells.write[child_idx].x = nx / half;
				bake_cells.write[child_idx].y = ny / half;
				bake_cells.write[child_idx].z = nz / half;
			}

			_plot_face(bake_cells[p_idx].children[i], p_level + 1, nx, ny, nz, p_vtx, p_normal,
				p_uv, p_material, aabb);
		}
	}
}

Vector<Color> Voxelizer::_get_bake_texture(
	Ref<Image> p_image, const Color& p_color_mul, const Color& p_color_add)
{
	Vector<Color> ret;

	if (p_image.is_null() || p_image->is_empty()) {
		ret.resize(bake_texture_size * bake_texture_size);
		for (int i = 0; i < bake_texture_size * bake_texture_size; i++) {
			ret.write[i] = p_color_add;
		}

		return ret;
	}
	if (p_image->is_compressed()) {
		p_image->decompress();
	}
	p_image->convert(Image::FORMAT_RGBA8);
	p_image->resize(bake_texture_size, bake_texture_size, Image::INTERPOLATE_CUBIC);

	const uint8_t* r = p_image->get_data().ptr();
	ret.resize(bake_texture_size * bake_texture_size);

	for (int i = 0; i < bake_texture_size * bake_texture_size; i++) {
		Color c;
		Color src = Color(
			r[i * 4 + 0] / 255.0, r[i * 4 + 1] / 255.0, r[i * 4 + 2] / 255.0, r[i * 4 + 3] / 255.0);
		src = src.srgb_to_linear();

		c.r = src.r * p_color_mul.r + p_color_add.r;
		c.g = src.g * p_color_mul.g + p_color_add.g;
		c.b = src.b * p_color_mul.b + p_color_add.b;
		c.a = src.a;

		ret.write[i] = c;
	}

	return ret;
}

Voxelizer::MaterialCache Voxelizer::_get_material_cache(Ref<Material> p_material)
{
	// This way of obtaining materials is inaccurate and also does not support some compressed
	// formats very well.
	Ref<StandardMaterial3D> mat = p_material;

	Ref<Material> material = mat; // hack for now

	if (material_cache.has(material)) {
		return material_cache[material];
	}

	MaterialCache mc;

	if (mat.is_valid()) {
		Ref<Texture2D> albedo_tex = mat->get_texture(StandardMaterial3D::TEXTURE_ALBEDO);

		Ref<Image> img_albedo;
		if (albedo_tex.is_valid()) {
			img_albedo = albedo_tex->get_image();
			mc.albedo = _get_bake_texture(img_albedo, mat->get_albedo().srgb_to_linear(),
				Color(0, 0, 0)); // albedo texture, color is multiplicative
		}
		else {
			mc.albedo = _get_bake_texture(img_albedo, Color(1, 1, 1),
				mat->get_albedo().srgb_to_linear()); // no albedo texture, color is additive
		}
		if (mat->get_feature(StandardMaterial3D::FEATURE_EMISSION)) {
			Ref<Texture2D> emission_tex = mat->get_texture(StandardMaterial3D::TEXTURE_EMISSION);

			Color emission_col = mat->get_emission().srgb_to_linear();
			float emission_energy = mat->get_emission_energy_multiplier() * exposure_normalization;
			if (GLOBAL_GET_CACHED(bool, "rendering/lights_and_shadows/use_physical_light_units")) {
				emission_energy *= mat->get_emission_intensity();
			}

			Ref<Image> img_emission;

			if (emission_tex.is_valid()) {
				img_emission = emission_tex->get_image();
			}

			if (mat->get_emission_operator() == StandardMaterial3D::EMISSION_OP_ADD) {
				mc.emission = _get_bake_texture(
					img_emission, Color(1, 1, 1) * emission_energy, emission_col * emission_energy);
			}
			else {
				mc.emission =
					_get_bake_texture(img_emission, emission_col * emission_energy, Color(0, 0, 0));
			}
		}
		else {
			Ref<Image> empty;
			mc.emission = _get_bake_texture(empty, Color(0, 0, 0), Color(0, 0, 0));
		}

	}
	else {
		Ref<Image> empty;

		mc.albedo = _get_bake_texture(empty, Color(0, 0, 0), Color(1, 1, 1));
		mc.emission = _get_bake_texture(empty, Color(0, 0, 0), Color(0, 0, 0));
	}

	material_cache[p_material] = mc;
	return mc;
}

void Voxelizer::_sort()
{
	// cells need to be sorted by level and coordinates
	// it is important that level has more priority (for compute), and that Z has the least,
	// given it may aid older implementations plot using GPU

	Vector<CellSort> sorted_cells;
	uint32_t cell_count = bake_cells.size();
	sorted_cells.resize(cell_count);
	{
		CellSort* sort_cellsp = sorted_cells.ptrw();
		const Cell* bake_cellsp = bake_cells.ptr();

		for (uint32_t i = 0; i < cell_count; i++) {
			sort_cellsp[i].x = bake_cellsp[i].x;
			sort_cellsp[i].y = bake_cellsp[i].y;
			sort_cellsp[i].z = bake_cellsp[i].z;
			sort_cellsp[i].level = bake_cellsp[i].level;
			sort_cellsp[i].index = i;
		}
	}

	sorted_cells.sort();

	// verify just in case, index 0 must be level 0
	ERR_FAIL_COND(sorted_cells[0].level != 0);

	Vector<Cell> new_bake_cells;
	new_bake_cells.resize(cell_count);
	Vector<uint32_t> reverse_map;

	{
		reverse_map.resize(cell_count);
		const CellSort* sort_cellsp = sorted_cells.ptr();
		uint32_t* reverse_mapp = reverse_map.ptrw();

		for (uint32_t i = 0; i < cell_count; i++) {
			reverse_mapp[sort_cellsp[i].index] = i;
		}
	}

	{
		const CellSort* sort_cellsp = sorted_cells.ptr();
		const Cell* bake_cellsp = bake_cells.ptr();
		const uint32_t* reverse_mapp = reverse_map.ptr();
		Cell* new_bake_cellsp = new_bake_cells.ptrw();

		for (uint32_t i = 0; i < cell_count; i++) {
			// copy to new cell
			new_bake_cellsp[i] = bake_cellsp[sort_cellsp[i].index];
			// remap children
			for (uint32_t j = 0; j < 8; j++) {
				if (new_bake_cellsp[i].children[j] != CHILD_EMPTY) {
					new_bake_cellsp[i].children[j] = reverse_mapp[new_bake_cellsp[i].children[j]];
				}
			}
		}
	}

	bake_cells = new_bake_cells;
	sorted = true;
}

void Voxelizer::_fixup_plot(int p_idx, int p_level)
{
	if (p_level == cell_subdiv) {
		leaf_voxel_count++;
		float alpha = bake_cells[p_idx].alpha;

		bake_cells.write[p_idx].albedo[0] /= alpha;
		bake_cells.write[p_idx].albedo[1] /= alpha;
		bake_cells.write[p_idx].albedo[2] /= alpha;

		// transfer emission to light
		bake_cells.write[p_idx].emission[0] /= alpha;
		bake_cells.write[p_idx].emission[1] /= alpha;
		bake_cells.write[p_idx].emission[2] /= alpha;

		bake_cells.write[p_idx].normal[0] /= alpha;
		bake_cells.write[p_idx].normal[1] /= alpha;
		bake_cells.write[p_idx].normal[2] /= alpha;

		Vector3 n(
			bake_cells[p_idx].normal[0], bake_cells[p_idx].normal[1], bake_cells[p_idx].normal[2]);
		if (n.length() < 0.01f) {
			// too much fight over normal, zero it
			bake_cells.write[p_idx].normal[0] = 0;
			bake_cells.write[p_idx].normal[1] = 0;
			bake_cells.write[p_idx].normal[2] = 0;
		}
		else {
			n.normalize();
			bake_cells.write[p_idx].normal[0] = n.x;
			bake_cells.write[p_idx].normal[1] = n.y;
			bake_cells.write[p_idx].normal[2] = n.z;
		}

		bake_cells.write[p_idx].alpha = 1.0;

		/*if (bake_light.size()) {
			for(int i=0;i<6;i++) {
			}
		}*/

	}
	else {
		// go down

		bake_cells.write[p_idx].emission[0] = 0;
		bake_cells.write[p_idx].emission[1] = 0;
		bake_cells.write[p_idx].emission[2] = 0;
		bake_cells.write[p_idx].normal[0] = 0;
		bake_cells.write[p_idx].normal[1] = 0;
		bake_cells.write[p_idx].normal[2] = 0;
		bake_cells.write[p_idx].albedo[0] = 0;
		bake_cells.write[p_idx].albedo[1] = 0;
		bake_cells.write[p_idx].albedo[2] = 0;

		float alpha_average = 0;

		for (int i = 0; i < 8; i++) {
			uint32_t child = bake_cells[p_idx].children[i];

			if (child == CHILD_EMPTY) {
				continue;
			}

			_fixup_plot(child, p_level + 1);
			alpha_average += bake_cells[child].alpha;
		}

		bake_cells.write[p_idx].alpha = alpha_average / 8.0;
	}
}

void Voxelizer::begin_bake(int p_subdiv, const AABB& p_bounds, float p_exposure_normalization)
{
	sorted = false;
	original_bounds = p_bounds;
	cell_subdiv = p_subdiv;
	exposure_normalization = p_exposure_normalization;
	bake_cells.resize(1);
	material_cache.clear();

	// find out the actual real bounds, power of 2, which gets the highest subdivision
	po2_bounds = p_bounds;
	int longest_axis = po2_bounds.get_longest_axis_index();
	axis_cell_size[longest_axis] = 1 << cell_subdiv;
	leaf_voxel_count = 0;

	for (int i = 0; i < 3; i++) {
		if (i == longest_axis) {
			continue;
		}

		axis_cell_size[i] = axis_cell_size[longest_axis];
		real_t axis_size = po2_bounds.size[longest_axis];

		// shrink until fit subdiv
		while (axis_size / 2.0 >= po2_bounds.size[i]) {
			axis_size /= 2.0;
			axis_cell_size[i] >>= 1;
		}

		po2_bounds.size[i] = po2_bounds.size[longest_axis];
	}

	Transform3D to_bounds;
	to_bounds.basis.scale(Vector3(po2_bounds.size[longest_axis], po2_bounds.size[longest_axis],
		po2_bounds.size[longest_axis]));
	to_bounds.origin = po2_bounds.position;

	Transform3D to_grid;
	to_grid.basis.scale(Vector3(
		axis_cell_size[longest_axis], axis_cell_size[longest_axis], axis_cell_size[longest_axis]));

	to_cell_space = to_grid * to_bounds.affine_inverse();

	cell_size = po2_bounds.size[longest_axis] / axis_cell_size[longest_axis];
}

void Voxelizer::end_bake()
{
	if (!sorted) {
		_sort();
	}
	_fixup_plot(0, 0);
}

// create the data for rendering server

int Voxelizer::get_voxel_gi_octree_depth() const { return cell_subdiv; }

Vector3i Voxelizer::get_voxel_gi_octree_size() const
{
	return Vector3i(axis_cell_size[0], axis_cell_size[1], axis_cell_size[2]);
}

int Voxelizer::get_voxel_gi_cell_count() const { return bake_cells.size(); }

Vector<uint8_t> Voxelizer::get_voxel_gi_octree_cells() const
{
	Vector<uint8_t> data;
	data.resize((8 * 4) * bake_cells.size()); // 8 uint32t values
	{
		uint8_t* w = data.ptrw();
		uint32_t* children_cells = (uint32_t*)w;
		const Cell* cells = bake_cells.ptr();

		uint32_t cell_count = bake_cells.size();

		for (uint32_t i = 0; i < cell_count; i++) {
			for (uint32_t j = 0; j < 8; j++) {
				children_cells[i * 8 + j] = cells[i].children[j];
			}
		}
	}

	return data;
}

Vector<uint8_t> Voxelizer::get_voxel_gi_data_cells() const
{
	Vector<uint8_t> data;
	data.resize((4 * 4) * bake_cells.size()); // 8 uint32t values
	{
		uint8_t* w = data.ptrw();
		uint32_t* dataptr = (uint32_t*)w;
		const Cell* cells = bake_cells.ptr();

		uint32_t cell_count = bake_cells.size();

		for (uint32_t i = 0; i < cell_count; i++) {
			{ // position

				uint32_t x = cells[i].x;
				uint32_t y = cells[i].y;
				uint32_t z = cells[i].z;

				uint32_t position = x;
				position |= y << 11;
				position |= z << 21;

				dataptr[i * 4 + 0] = position;
			}

			{ // albedo + alpha
				uint32_t rgba = uint32_t(CLAMP(cells[i].alpha * 255.0, 0, 255)) << 24; // a
				rgba |= uint32_t(CLAMP(cells[i].albedo[2] * 255.0, 0, 255)) << 16;	   // b
				rgba |= uint32_t(CLAMP(cells[i].albedo[1] * 255.0, 0, 255)) << 8;	   // g
				rgba |= uint32_t(CLAMP(cells[i].albedo[0] * 255.0, 0, 255));		   // r

				dataptr[i * 4 + 1] = rgba;
			}

			{ // emission, as rgbe9995
				Color emission =
					Color(cells[i].emission[0], cells[i].emission[1], cells[i].emission[2]);
				dataptr[i * 4 + 2] = emission.to_rgbe9995();
			}

			{ // normal

				Vector3 n(
					bake_cells[i].normal[0], bake_cells[i].normal[1], bake_cells[i].normal[2]);
				n.normalize();

				uint32_t normal = uint32_t(uint8_t(int8_t(CLAMP(n.x * 127.0, -128, 127))));
				normal |= uint32_t(uint8_t(int8_t(CLAMP(n.y * 127.0, -128, 127)))) << 8;
				normal |= uint32_t(uint8_t(int8_t(CLAMP(n.z * 127.0, -128, 127)))) << 16;

				dataptr[i * 4 + 3] = normal;
			}
		}
	}

	return data;
}

Vector<int> Voxelizer::get_voxel_gi_level_cell_count() const
{
	uint32_t cell_count = bake_cells.size();
	const Cell* cells = bake_cells.ptr();
	Vector<int> level_count;
	level_count.resize(cell_subdiv + 1); // remember, always x+1 levels for x subdivisions
	{
		int* w = level_count.ptrw();
		for (int i = 0; i < cell_subdiv + 1; i++) {
			w[i] = 0;
		}

		for (uint32_t i = 0; i < cell_count; i++) {
			w[cells[i].level]++;
		}
	}

	return level_count;
}

// euclidean distance computation based on:
// https://prideout.net/blog/distance_fields/

#define square(m_s) ((m_s) * (m_s))
#define BIG_VAL 1e20

/* dt of 1d function using squared distance */
static void edt(float* f, int stride, int n)
{
	float* d = (float*)alloca(sizeof(float) * n + sizeof(int) * n + sizeof(float) * (n + 1));
	int* v = reinterpret_cast<int*>(&(d[n]));
	float* z = reinterpret_cast<float*>(&v[n]);

	int k = 0;
	v[0] = 0;
	z[0] = -BIG_VAL;
	z[1] = +BIG_VAL;
	for (int q = 1; q <= n - 1; q++) {
		float s =
			((f[q * stride] + square(q)) - (f[v[k] * stride] + square(v[k]))) / (2 * q - 2 * v[k]);
		while (s <= z[k]) {
			k--;
			s = ((f[q * stride] + square(q)) - (f[v[k] * stride] + square(v[k]))) /
				(2 * q - 2 * v[k]);
		}
		k++;
		v[k] = q;

		z[k] = s;
		z[k + 1] = +BIG_VAL;
	}

	k = 0;
	for (int q = 0; q <= n - 1; q++) {
		while (z[k + 1] < q) {
			k++;
		}
		d[q] = square(q - v[k]) + f[v[k] * stride];
	}

	for (int i = 0; i < n; i++) {
		f[i * stride] = d[i];
	}
}

#undef square

Voxelizer::BakeResult Voxelizer::get_sdf_3d_image(
	Vector<uint8_t>& r_image, BakeStepFunc p_bake_step_function) const
{
	Vector3i octree_size = get_voxel_gi_octree_size();

	uint32_t float_count = octree_size.x * octree_size.y * octree_size.z;
	float* work_memory = memnew_arr(float, float_count);
	for (uint32_t i = 0; i < float_count; i++) {
		work_memory[i] = BIG_VAL;
	}

	uint32_t y_mult = octree_size.x;
	uint32_t z_mult = y_mult * octree_size.y;

	// plot solid cells
	{
		const Cell* cells = bake_cells.ptr();
		uint32_t cell_count = bake_cells.size();

		for (uint32_t i = 0; i < cell_count; i++) {
			if (cells[i].level < cell_subdiv) {
				continue; // do not care about this level
			}

			work_memory[cells[i].x + cells[i].y * y_mult + cells[i].z * z_mult] = 0;
		}
	}

	// process in each direction

	int bake_total = octree_size.x * 2 + octree_size.y, bake_current = 0;

	// xy->z

	for (int i = 0; i < octree_size.x; i++, bake_current++) {
		if (p_bake_step_function) {
			if (p_bake_step_function(bake_current, bake_total)) {
				memdelete_arr(work_memory);
				return BAKE_RESULT_CANCELLED;
			}
		}
		for (int j = 0; j < octree_size.y; j++) {
			edt(&work_memory[i + j * y_mult], z_mult, octree_size.z);
		}
	}

	// xz->y

	for (int i = 0; i < octree_size.x; i++, bake_current++) {
		if (p_bake_step_function) {
			if (p_bake_step_function(bake_current, bake_total)) {
				memdelete_arr(work_memory);
				return BAKE_RESULT_CANCELLED;
			}
		}
		for (int j = 0; j < octree_size.z; j++) {
			edt(&work_memory[i + j * z_mult], y_mult, octree_size.y);
		}
	}

	// yz->x
	for (int i = 0; i < octree_size.y; i++, bake_current++) {
		if (p_bake_step_function) {
			if (p_bake_step_function(bake_current, bake_total)) {
				memdelete_arr(work_memory);
				return BAKE_RESULT_CANCELLED;
			}
		}
		for (int j = 0; j < octree_size.z; j++) {
			edt(&work_memory[i * y_mult + j * z_mult], 1, octree_size.x);
		}
	}

	r_image.resize(float_count);
	{
		uint8_t* w = r_image.ptrw();
		for (uint32_t i = 0; i < float_count; i++) {
			uint32_t d = uint32_t(Math::sqrt(work_memory[i]));
			if (d == 0) {
				w[i] = 0;
			}
			else {
				w[i] = MIN(d, 254u) + 1;
			}
		}
	}

	memdelete_arr(work_memory);

	return BAKE_RESULT_OK;
}

#undef BIG_VAL

void Voxelizer::_debug_mesh(
	int p_idx, int p_level, const AABB& p_aabb, Ref<MultiMesh>& p_multimesh, int& idx)
{
	if (p_level == cell_subdiv - 1) {
		Vector3 center = p_aabb.get_center();
		Transform3D xform;
		xform.origin = center;
		xform.basis.scale(p_aabb.size * 0.5);
		p_multimesh->set_instance_transform(idx, xform);
		Color col;
		col = Color(
			bake_cells[p_idx].albedo[0], bake_cells[p_idx].albedo[1], bake_cells[p_idx].albedo[2]);
		// Color col = Color(bake_cells[p_idx].emission[0], bake_cells[p_idx].emission[1],
		// bake_cells[p_idx].emission[2]);
		p_multimesh->set_instance_color(idx, col);

		idx++;

	}
	else {
		for (int i = 0; i < 8; i++) {
			uint32_t child = bake_cells[p_idx].children[i];

			if (child == CHILD_EMPTY || child >= (uint32_t)max_original_cells) {
				continue;
			}

			AABB aabb = p_aabb;
			aabb.size *= 0.5;

			if (i & 1) {
				aabb.position.x += aabb.size.x;
			}
			if (i & 2) {
				aabb.position.y += aabb.size.y;
			}
			if (i & 4) {
				aabb.position.z += aabb.size.z;
			}

			_debug_mesh(bake_cells[p_idx].children[i], p_level + 1, aabb, p_multimesh, idx);
		}
	}
}

Transform3D Voxelizer::get_to_cell_space_xform() const { return to_cell_space; }

Voxelizer::Voxelizer() {}

// ==========================================================================
// Merged from scene/3d/lightmap_gi.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/math/geometry_3d.h"
// --- Inlined from scene/3d/lightmap_gi.h ---
#include "core/io/resource.h"
#include "core/templates/local_vector.h"
#include "scene/3d/visual_instance_3d.h"

class CameraAttributes;
class Light3D;
class Mesh;
class Sky;

class LightmapGIData : public Resource
{
public:
	enum ShadowmaskMode
	{
		SHADOWMASK_MODE_NONE,
		SHADOWMASK_MODE_REPLACE,
		SHADOWMASK_MODE_OVERLAY,
		SHADOWMASK_MODE_ONLY,
	};

private:
	// The 'merged' texture atlases actually used by the renderer.
	Ref<TextureLayered> combined_light_texture;
	Ref<TextureLayered> combined_shadowmask_texture;

	bool uses_spherical_harmonics = false;
	bool interior = false;

	bool _uses_packed_directional = false;

	RID lightmap;
	AABB bounds;
	float baked_exposure = 1.0;
	uint32_t lightprobe_hash = 0;

	struct User
	{
		NodePath path;
		int32_t sub_instance = 0;
		Rect2 uv_scale;
		int slice_index = 0;
	};

	Vector<User> users;

	void _reset_lightmap_textures();
	void _reset_shadowmask_textures();

protected:

public:
	void add_user(const NodePath& p_path, const Rect2& p_uv_scale, int p_slice_index,
		int32_t p_sub_instance = -1);
	int get_user_count() const;
	NodePath get_user_path(int p_user) const;
	int32_t get_user_sub_instance(int p_user) const;
	Rect2 get_user_lightmap_uv_scale(int p_user) const;
	int get_user_lightmap_slice_index(int p_user) const;
	void clear_users();

#ifndef DISABLE_DEPRECATED
	void set_light_texture(const Ref<TextureLayered>& p_light_texture);
	Ref<TextureLayered> get_light_texture() const;
#endif

	void set_uses_spherical_harmonics(bool p_enable);
	bool is_using_spherical_harmonics() const;

	void _set_uses_packed_directional(bool p_enable);
	bool _is_using_packed_directional() const;

	void update_shadowmask_mode(ShadowmaskMode p_mode);
	ShadowmaskMode get_shadowmask_mode() const;

	bool is_interior() const;
	float get_baked_exposure() const;

	void set_capture_data(const AABB& p_bounds, bool p_interior, const PackedVector3Array& p_points,
		const PackedColorArray& p_point_sh, const PackedInt32Array& p_tetrahedra,
		const PackedInt32Array& p_bsp_tree, float p_baked_exposure, uint32_t p_lightprobe_hash);
	PackedVector3Array get_capture_points() const;
	PackedColorArray get_capture_sh() const;
	PackedInt32Array get_capture_tetrahedra() const;
	PackedInt32Array get_capture_bsp_tree() const;
	uint32_t get_lightprobe_hash() const;

	AABB get_capture_bounds() const;

	void clear();

	void clear_shadowmask_textures();
	bool has_shadowmask_textures();

	virtual RID get_rid() const override;
	LightmapGIData();
	~LightmapGIData();
};

class LightmapGI : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum BakeQuality
	{
		BAKE_QUALITY_LOW,
		BAKE_QUALITY_MEDIUM,
		BAKE_QUALITY_HIGH,
		BAKE_QUALITY_ULTRA,
	};

	enum GenerateProbes
	{
		GENERATE_PROBES_DISABLED,
		GENERATE_PROBES_SUBDIV_4,
		GENERATE_PROBES_SUBDIV_8,
		GENERATE_PROBES_SUBDIV_16,
		GENERATE_PROBES_SUBDIV_32,
	};

	enum BakeError
	{
		BAKE_ERROR_OK,
		BAKE_ERROR_NO_SCENE_ROOT,
		BAKE_ERROR_FOREIGN_DATA,
		BAKE_ERROR_NO_LIGHTMAPPER,
		BAKE_ERROR_NO_SAVE_PATH,
		BAKE_ERROR_NO_MESHES,
		BAKE_ERROR_MESHES_INVALID,
		BAKE_ERROR_CANT_CREATE_IMAGE,
		BAKE_ERROR_USER_ABORTED,
		BAKE_ERROR_TEXTURE_SIZE_TOO_SMALL,
		BAKE_ERROR_LIGHTMAP_TOO_SMALL,
		BAKE_ERROR_ATLAS_TOO_SMALL,
	};

	enum EnvironmentMode
	{
		ENVIRONMENT_MODE_DISABLED,
		ENVIRONMENT_MODE_SCENE,
		ENVIRONMENT_MODE_CUSTOM_SKY,
		ENVIRONMENT_MODE_CUSTOM_COLOR,
	};

private:
	BakeQuality bake_quality = BAKE_QUALITY_MEDIUM;
	bool use_denoiser = true;
	float denoiser_strength = 0.1f;
	int denoiser_range = 10;
	int bounces = 3;
	float bounce_indirect_energy = 1.0;
	float bias = 0.0005;
	float texel_scale = 1.0;
	int max_texture_size = 16384;
	bool supersampling_enabled = false;
	float supersampling_factor = 2.0;
	bool interior = false;
	EnvironmentMode environment_mode = ENVIRONMENT_MODE_SCENE;
	Ref<Sky> environment_custom_sky;
	Color environment_custom_color = Color(1, 1, 1);
	float environment_custom_energy = 1.0;
	bool directional = false;
	bool use_texture_for_bounces = true;
	LightmapGIData::ShadowmaskMode shadowmask_mode = LightmapGIData::SHADOWMASK_MODE_NONE;
	GenerateProbes gen_probes = GENERATE_PROBES_SUBDIV_8;
	Ref<CameraAttributes> camera_attributes;

	Ref<LightmapGIData> light_data;
	Node* last_owner = nullptr;

	struct LightsFound
	{
		Transform3D xform;
		Light3D* light = nullptr;
	};

	struct AreaLightAtlasTexture
	{
		Rect2 texture_rect;
		float max_mipmap;
	};

	struct MeshesFound
	{
		Transform3D xform;
		NodePath node_path;
		int32_t subindex = 0;
		Ref<Mesh> mesh;
		float lightmap_scale = 0.0;
		Vector<Ref<Material>> overrides;
	};

	void _find_meshes_and_lights(Node* p_at_node, Vector<MeshesFound>& meshes,
		Vector<LightsFound>& lights, Vector<Vector3>& probes);

	void _assign_lightmaps();
	void _clear_lightmaps();

	struct BSPSimplex
	{
		int vertices[4] = {};
		int planes[4] = {};
	};

	struct BSPNode
	{
		static const int32_t EMPTY_LEAF = INT32_MIN;
		Plane plane;
		int32_t over = EMPTY_LEAF;
		int32_t under = EMPTY_LEAF;
	};

	int _bsp_get_simplex_side(const LocalVector<Vector3>& p_points,
		const LocalVector<BSPSimplex>& p_simplices, const Plane& p_plane, uint32_t p_simplex) const;
	int32_t _compute_bsp_tree(const LocalVector<Vector3>& p_points,
		const LocalVector<Plane>& p_planes, LocalVector<int32_t>& planes_tested,
		const LocalVector<BSPSimplex>& p_simplices, const LocalVector<int32_t>& p_simplex_indices,
		LocalVector<BSPNode>& bsp_nodes);

	struct BakeStepUD
	{
		Lightmapper::BakeStepFunc func;
		void* ud = nullptr;
		float from_percent = 0.0;
		float to_percent = 0.0;
	};

	static bool _lightmap_bake_step_function(
		float p_completion, const String& p_text, void* ud, bool p_refresh);

	struct GenProbesOctree
	{
		Vector3i offset;
		uint32_t size = 0;
		GenProbesOctree* children[8] = {
			nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

		~GenProbesOctree()
		{
			for (int i = 0; i < 8; i++) {
				memdelete(children[i]);
			}
		}
	};

	void _plot_triangle_into_octree(
		GenProbesOctree* p_cell, float p_cell_size, const Vector3* p_triangle);
	void _gen_new_positions_from_octree(const GenProbesOctree* p_cell, float p_cell_size,
		const Vector<Vector3>& probe_positions, LocalVector<Vector3>& new_probe_positions,
		HashMap<Vector3i, bool>& positions_used, const AABB& p_bounds);

	void _build_area_light_texture_atlas(const Vector<LightmapGI::LightsFound>& lights_found,
		HashMap<Ref<Texture2D>, AreaLightAtlasTexture>& r_texture_rects, Size2i& r_atlas_size,
		int& r_mipmaps) const;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	Ref<LightmapGIData> get_light_data() const;

	void set_bake_quality(BakeQuality p_quality);
	BakeQuality get_bake_quality() const;

	void set_use_denoiser(bool p_enable);
	bool is_using_denoiser() const;

	void set_denoiser_strength(float p_denoiser_strength);
	float get_denoiser_strength() const;

	void set_denoiser_range(int p_denoiser_range);
	int get_denoiser_range() const;

	void set_directional(bool p_enable);
	bool is_directional() const;

	LightmapGIData::ShadowmaskMode get_shadowmask_mode() const;

	void set_use_texture_for_bounces(bool p_enable);
	bool is_using_texture_for_bounces() const;

	void set_interior(bool p_interior);
	bool is_interior() const;

	void set_environment_mode(EnvironmentMode p_mode);
	EnvironmentMode get_environment_mode() const;

	void set_environment_custom_sky(const Ref<Sky>& p_sky);
	Ref<Sky> get_environment_custom_sky() const;

	void set_environment_custom_color(const Color& p_color);
	Color get_environment_custom_color() const;

	void set_environment_custom_energy(float p_energy);
	float get_environment_custom_energy() const;

	void set_bounces(int p_bounces);
	int get_bounces() const;

	void set_bounce_indirect_energy(float p_indirect_energy);
	float get_bounce_indirect_energy() const;

	void set_bias(float p_bias);
	float get_bias() const;

	void set_texel_scale(float p_multiplier);
	float get_texel_scale() const;

	void set_max_texture_size(int p_size);
	int get_max_texture_size() const;

	void set_supersampling_enabled(bool p_enable);
	bool is_supersampling_enabled() const;

	void set_supersampling_factor(float p_factor);
	float get_supersampling_factor() const;

	void set_generate_probes(GenerateProbes p_generate_probes);
	GenerateProbes get_generate_probes() const;

	void set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes);
	Ref<CameraAttributes> get_camera_attributes() const;

	AABB get_aabb() const;

	BakeError bake(Node* p_from_node, String p_image_data_path = "",
		Lightmapper::BakeStepFunc p_bake_step = nullptr, void* p_bake_userdata = nullptr);

	virtual PackedStringArray get_configuration_warnings() const override;

	LightmapGI();
};



// --- End scene/3d/lightmap_gi.h ---
#include "modules/modules_enabled.gen.h" // IWYU pragma: keep. For lightmapper_rd.
#include "scene/3d/visual_instance_3d.h"
// --- Inlined from scene/3d/lightmap_probe.h ---
#include "scene/3d/node_3d.h"

class LightmapProbe : public Node3D
{
public:
	LightmapProbe();
};



// --- End scene/3d/lightmap_probe.h ---
#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/material.h"
#include "scene/resources/environment.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/environment.h"
#include "servers/rendering/renderer.h"

#ifdef MODULE_LIGHTMAPPER_RD_ENABLED
#include "servers/display/display_server.h"
#endif

#if defined(ANDROID_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
#include "core/os/os.h"
#endif

void LightmapGIData::add_user(
	const NodePath& p_path, const Rect2& p_uv_scale, int p_slice_index, int32_t p_sub_instance)
{
	User user;
	user.path = p_path;
	user.uv_scale = p_uv_scale;
	user.slice_index = p_slice_index;
	user.sub_instance = p_sub_instance;
	users.push_back(user);
}

int LightmapGIData::get_user_count() const { return users.size(); }

NodePath LightmapGIData::get_user_path(int p_user) const
{
	ERR_FAIL_INDEX_V(p_user, users.size(), NodePath());
	return users[p_user].path;
}

int32_t LightmapGIData::get_user_sub_instance(int p_user) const
{
	ERR_FAIL_INDEX_V(p_user, users.size(), -1);
	return users[p_user].sub_instance;
}

Rect2 LightmapGIData::get_user_lightmap_uv_scale(int p_user) const
{
	ERR_FAIL_INDEX_V(p_user, users.size(), Rect2());
	return users[p_user].uv_scale;
}

int LightmapGIData::get_user_lightmap_slice_index(int p_user) const
{
	ERR_FAIL_INDEX_V(p_user, users.size(), -1);
	return users[p_user].slice_index;
}

void LightmapGIData::clear_users() { users.clear(); }

RID LightmapGIData::get_rid() const { return lightmap; }

void LightmapGIData::clear() { users.clear(); }

void LightmapGIData::_reset_lightmap_textures()
{
	Renderer::lightmap_set_textures(lightmap,
		combined_light_texture.is_valid() ? combined_light_texture->get_rid() : RID(),
		uses_spherical_harmonics);
}

void LightmapGIData::_reset_shadowmask_textures()
{
	Renderer::lightmap_set_shadowmask_textures(lightmap,
		combined_shadowmask_texture.is_valid() ? combined_shadowmask_texture->get_rid() : RID());
}

void LightmapGIData::set_uses_spherical_harmonics(bool p_enable)
{
	uses_spherical_harmonics = p_enable;
	_reset_lightmap_textures();
}

bool LightmapGIData::is_using_spherical_harmonics() const { return uses_spherical_harmonics; }

void LightmapGIData::_set_uses_packed_directional(bool p_enable)
{
	_uses_packed_directional = p_enable;
}

bool LightmapGIData::_is_using_packed_directional() const { return _uses_packed_directional; }

void LightmapGIData::update_shadowmask_mode(ShadowmaskMode p_mode)
{
	Renderer::lightmap_set_shadowmask_mode(lightmap, (RSE::ShadowmaskMode)p_mode);
}

LightmapGIData::ShadowmaskMode LightmapGIData::get_shadowmask_mode() const
{
	return (ShadowmaskMode)Renderer::lightmap_get_shadowmask_mode(lightmap);
}

void LightmapGIData::set_capture_data(const AABB& p_bounds, bool p_interior,
	const PackedVector3Array& p_points, const PackedColorArray& p_point_sh,
	const PackedInt32Array& p_tetrahedra, const PackedInt32Array& p_bsp_tree,
	float p_baked_exposure, uint32_t p_lightprobe_hash)
{
	if (p_points.size()) {
		int pc = p_points.size();
		ERR_FAIL_COND(pc * 9 != p_point_sh.size());
		ERR_FAIL_COND((p_tetrahedra.size() % 4) != 0);
		ERR_FAIL_COND((p_bsp_tree.size() % 6) != 0);
		Renderer::lightmap_set_probe_capture_data(
			lightmap, p_points, p_point_sh, p_tetrahedra, p_bsp_tree);
		Renderer::lightmap_set_probe_bounds(lightmap, p_bounds);
		Renderer::lightmap_set_probe_interior(lightmap, p_interior);
	}
	else {
		Renderer::lightmap_set_probe_capture_data(lightmap, PackedVector3Array(),
			PackedColorArray(), PackedInt32Array(), PackedInt32Array());
		Renderer::lightmap_set_probe_bounds(lightmap, AABB());
		Renderer::lightmap_set_probe_interior(lightmap, false);
	}
	Renderer::lightmap_set_baked_exposure_normalization(lightmap, p_baked_exposure);
	baked_exposure = p_baked_exposure;
	lightprobe_hash = p_lightprobe_hash;
	interior = p_interior;
	bounds = p_bounds;
}

PackedVector3Array LightmapGIData::get_capture_points() const
{
	return Renderer::lightmap_get_probe_capture_points(lightmap);
}

PackedColorArray LightmapGIData::get_capture_sh() const
{
	return Renderer::lightmap_get_probe_capture_sh(lightmap);
}

PackedInt32Array LightmapGIData::get_capture_tetrahedra() const
{
	return Renderer::lightmap_get_probe_capture_tetrahedra(lightmap);
}

PackedInt32Array LightmapGIData::get_capture_bsp_tree() const
{
	return Renderer::lightmap_get_probe_capture_bsp_tree(lightmap);
}

uint32_t LightmapGIData::get_lightprobe_hash() const { return lightprobe_hash; }

AABB LightmapGIData::get_capture_bounds() const { return bounds; }

bool LightmapGIData::is_interior() const { return interior; }

float LightmapGIData::get_baked_exposure() const { return baked_exposure; }

LightmapGIData::LightmapGIData() { lightmap = Renderer::lightmap_create(); }

LightmapGIData::~LightmapGIData()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(lightmap);
}

///////////////////////////

int LightmapGI::_bsp_get_simplex_side(const LocalVector<Vector3>& p_points,
	const LocalVector<BSPSimplex>& p_simplices, const Plane& p_plane, uint32_t p_simplex) const
{
	int over = 0;
	int under = 0;
	const BSPSimplex& s = p_simplices[p_simplex];
	for (int i = 0; i < 4; i++) {
		const Vector3 v = p_points[s.vertices[i]];
		// The tolerance used here comes from experiments on scenes up to
		// 1000x1000x100 meters. If it's any smaller, some simplices will
		// appear to self-intersect due to a lack of precision in Plane.
		if (p_plane.has_point(v, 1.0 / (1 << 13))) {
			// Coplanar.
		}
		else if (p_plane.is_point_over(v)) {
			over++;
		}
		else {
			under++;
		}
	}

	ERR_FAIL_COND_V(under == 0 && over == 0,
		-2); // should never happen, we discarded flat simplices before, but in any case drop it
			 // from the bsp tree and throw an error
	if (under == 0) {
		return 1; // all over
	}
	else if (over == 0) {
		return -1; // all under
	}
	else {
		return 0; // crossing
	}
}

// #define DEBUG_BSP

int32_t LightmapGI::_compute_bsp_tree(const LocalVector<Vector3>& p_points,
	const LocalVector<Plane>& p_planes, LocalVector<int32_t>& planes_tested,
	const LocalVector<BSPSimplex>& p_simplices, const LocalVector<int32_t>& p_simplex_indices,
	LocalVector<BSPNode>& bsp_nodes)
{
	ERR_FAIL_COND_V(p_simplex_indices.size() < 2, -1);

	int32_t node_index = (int32_t)bsp_nodes.size();
	bsp_nodes.push_back(BSPNode());

	// test with all the simplex planes
	Plane best_plane;
	float best_plane_score = -1.0;

	for (const int idx : p_simplex_indices) {
		const BSPSimplex& s = p_simplices[idx];
		for (int j = 0; j < 4; j++) {
			uint32_t plane_index = s.planes[j];
			if (planes_tested[plane_index] == node_index) {
				continue; // tested this plane already
			}

			planes_tested[plane_index] = node_index;

			static const int face_order[4][3] = {{0, 1, 2}, {0, 2, 3}, {0, 1, 3}, {1, 2, 3}};

			// despite getting rid of plane duplicates, we should still use here the actual plane to
			// avoid numerical error from thinking this same simplex is intersecting rather than on
			// a side
			Vector3 v0 = p_points[s.vertices[face_order[j][0]]];
			Vector3 v1 = p_points[s.vertices[face_order[j][1]]];
			Vector3 v2 = p_points[s.vertices[face_order[j][2]]];

			Plane plane(v0, v1, v2);

			// test with all the simplices
			int over_count = 0;
			int under_count = 0;

			for (const int& index : p_simplex_indices) {
				int side = _bsp_get_simplex_side(p_points, p_simplices, plane, index);
				if (side == -2) {
					continue; // this simplex is invalid, skip for now
				}
				else if (side < 0) {
					under_count++;
				}
				else if (side > 0) {
					over_count++;
				}
			}

			if (under_count == 0 && over_count == 0) {
				continue; // most likely precision issue with a flat simplex, do not try this plane
			}

			if (under_count > over_count) { // make sure under is always less than over, so we can
											// compute the same ratio
				SWAP(under_count, over_count);
			}

			float score = 0; // by default, score is 0 (worst)
			if (over_count > 0) {
				// Simplices that are intersected by the plane are moved into both the over
				// and under subtrees which makes the entire tree deeper, so the best plane
				// will have the least intersections while separating the simplices evenly.
				float balance = float(under_count) / over_count;
				float separation = float(over_count + under_count) / p_simplex_indices.size();
				score = balance * separation * separation;
			}

			if (score > best_plane_score) {
				best_plane = plane;
				best_plane_score = score;
			}
		}
	}

	// We often end up with two (or on rare occasions, three) simplices that are
	// either disjoint or share one vertex and don't have a separating plane
	// among their faces. The fallback is to loop through new planes created
	// with one vertex of the first simplex and two vertices of the second until
	// we find a winner.
	if (best_plane_score == 0) {
		const BSPSimplex& simplex0 = p_simplices[p_simplex_indices[0]];
		const BSPSimplex& simplex1 = p_simplices[p_simplex_indices[1]];

		for (uint32_t i = 0; i < 4 && !best_plane_score; i++) {
			Vector3 v0 = p_points[simplex0.vertices[i]];
			for (uint32_t j = 0; j < 3 && !best_plane_score; j++) {
				if (simplex0.vertices[i] == simplex1.vertices[j]) {
					break;
				}
				Vector3 v1 = p_points[simplex1.vertices[j]];
				for (uint32_t k = j + 1; k < 4; k++) {
					if (simplex0.vertices[i] == simplex1.vertices[k]) {
						break;
					}
					Vector3 v2 = p_points[simplex1.vertices[k]];

					Plane plane = Plane(v0, v1, v2);
					if (plane ==
						Plane()) { // When v0, v1, and v2 are collinear, they can't form a plane.
						continue;
					}
					int32_t side0 =
						_bsp_get_simplex_side(p_points, p_simplices, plane, p_simplex_indices[0]);
					int32_t side1 =
						_bsp_get_simplex_side(p_points, p_simplices, plane, p_simplex_indices[1]);
					if ((side0 == 1 && side1 == -1) || (side0 == -1 && side1 == 1)) {
						best_plane = plane;
						best_plane_score = 1.0;
						break;
					}
				}
			}
		}
	}

	LocalVector<int32_t> indices_over;
	LocalVector<int32_t> indices_under;

	// split again, but add to list
	for (const uint32_t index : p_simplex_indices) {
		int side = _bsp_get_simplex_side(p_points, p_simplices, best_plane, index);

		if (side == -2) {
			continue; // simplex sits on the plane, does not make sense to use it
		}
		if (side <= 0) {
			indices_under.push_back(index);
		}

		if (side >= 0) {
			indices_over.push_back(index);
		}
	}

#ifdef DEBUG_BSP
	print_line("node " + itos(node_index) + " found plane: " + best_plane +
			   " score:" + rtos(best_plane_score) + " - over " + itos(indices_over.size()) +
			   " under " + itos(indices_under.size()) + " intersecting " + itos(intersecting));
#endif

	if (best_plane_score < 0.0 || indices_over.size() == p_simplex_indices.size() ||
		indices_under.size() == p_simplex_indices.size()) {
		// Failed to separate the tetrahedrons using planes
		// this means Delaunay broke at some point.
		// Luckily, because we are using tetrahedrons, we can resort to
		// less precise but still working ways to generate the separating plane
		// this will most likely look bad when interpolating, but at least it will not crash.
		// and the artifact will most likely also be very small, so too difficult to notice.

		// find the longest axis

		WARN_PRINT("Inconsistency found in triangulation while building BSP, probe interpolation "
				   "quality may degrade a bit.");

		LocalVector<Vector3> centers;
		AABB bounds_all;
		for (uint32_t i = 0; i < p_simplex_indices.size(); i++) {
			AABB bounds;
			for (uint32_t j = 0; j < 4; j++) {
				Vector3 p = p_points[p_simplices[p_simplex_indices[i]].vertices[j]];
				if (j == 0) {
					bounds.position = p;
				}
				else {
					bounds.expand_to(p);
				}
			}
			if (i == 0) {
				centers.push_back(bounds.get_center());
			}
			else {
				bounds_all.merge_with(bounds);
			}
		}
		Vector3::Axis longest_axis = Vector3::Axis(bounds_all.get_longest_axis_index());

		// find the simplex that will go under
		uint32_t min_d_idx = 0xFFFFFFFF;
		float min_d_dist = 1e20;

		for (uint32_t i = 0; i < centers.size(); i++) {
			if (centers[i][longest_axis] < min_d_dist) {
				min_d_idx = i;
				min_d_dist = centers[i][longest_axis];
			}
		}
		// rebuild best_plane and over/under arrays
		best_plane = Plane();
		best_plane.normal[longest_axis] = 1.0;
		best_plane.d = min_d_dist;

		indices_under.clear();
		indices_under.push_back(min_d_idx);

		indices_over.clear();

		for (uint32_t i = 0; i < p_simplex_indices.size(); i++) {
			if (i == min_d_idx) {
				continue;
			}
			indices_over.push_back(p_simplex_indices[i]);
		}
	}

	BSPNode node;
	node.plane = best_plane;

	if (indices_under.is_empty()) {
		// nothing to do here
		node.under = BSPNode::EMPTY_LEAF;
	}
	else if (indices_under.size() == 1) {
		node.under = -(indices_under[0] + 1);
	}
	else {
		node.under = _compute_bsp_tree(
			p_points, p_planes, planes_tested, p_simplices, indices_under, bsp_nodes);
	}

	if (indices_over.is_empty()) {
		// nothing to do here
		node.over = BSPNode::EMPTY_LEAF;
	}
	else if (indices_over.size() == 1) {
		node.over = -(indices_over[0] + 1);
	}
	else {
		node.over = _compute_bsp_tree(
			p_points, p_planes, planes_tested, p_simplices, indices_over, bsp_nodes);
	}

	bsp_nodes[node_index] = node;

	return node_index;
}

bool LightmapGI::_lightmap_bake_step_function(
	float p_completion, const String& p_text, void* ud, bool p_refresh)
{
	BakeStepUD* bsud = (BakeStepUD*)ud;
	bool ret = false;
	if (bsud->func) {
		ret =
			bsud->func(bsud->from_percent + p_completion * (bsud->to_percent - bsud->from_percent),
				p_text, bsud->ud, p_refresh);
	}
	return ret;
}

void LightmapGI::_plot_triangle_into_octree(
	GenProbesOctree* p_cell, float p_cell_size, const Vector3* p_triangle)
{
	for (int i = 0; i < 8; i++) {
		Vector3i pos = p_cell->offset;
		uint32_t half_size = p_cell->size / 2;
		if (i & 1) {
			pos.x += half_size;
		}
		if (i & 2) {
			pos.y += half_size;
		}
		if (i & 4) {
			pos.z += half_size;
		}

		AABB subcell;
		subcell.position = Vector3(pos) * p_cell_size;
		subcell.size = Vector3(half_size, half_size, half_size) * p_cell_size;

		if (!Geometry3D::triangle_box_overlap(
				subcell.get_center(), subcell.size * 0.5, p_triangle)) {
			continue;
		}

		if (p_cell->children[i] == nullptr) {
			GenProbesOctree* child = memnew(GenProbesOctree);
			child->offset = pos;
			child->size = half_size;
			p_cell->children[i] = child;
		}

		if (half_size > 1) {
			// still levels missing
			_plot_triangle_into_octree(p_cell->children[i], p_cell_size, p_triangle);
		}
	}
}

void LightmapGI::_gen_new_positions_from_octree(const GenProbesOctree* p_cell, float p_cell_size,
	const Vector<Vector3>& probe_positions, LocalVector<Vector3>& new_probe_positions,
	HashMap<Vector3i, bool>& positions_used, const AABB& p_bounds)
{
	for (int i = 0; i < 8; i++) {
		Vector3i pos = p_cell->offset;
		if (i & 1) {
			pos.x += p_cell->size;
		}
		if (i & 2) {
			pos.y += p_cell->size;
		}
		if (i & 4) {
			pos.z += p_cell->size;
		}

		if (p_cell->size == 1 && !positions_used.has(pos)) {
			// new position to insert!
			Vector3 real_pos = p_bounds.position + Vector3(pos) * p_cell_size;
			// see if a user submitted probe is too close
			int ppcount = probe_positions.size();
			const Vector3* pp = probe_positions.ptr();
			bool exists = false;
			for (int j = 0; j < ppcount; j++) {
				if (pp[j].distance_to(real_pos) < (p_cell_size * 0.5f)) {
					exists = true;
					break;
				}
			}

			if (!exists) {
				new_probe_positions.push_back(real_pos);
			}

			positions_used[pos] = true;
		}

		if (p_cell->children[i] != nullptr) {
			_gen_new_positions_from_octree(p_cell->children[i], p_cell_size, probe_positions,
				new_probe_positions, positions_used, p_bounds);
		}
	}
}

void LightmapGI::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void LightmapGI::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_POST_ENTER_TREE: {
		if (light_data.is_valid()) {
			ERR_FAIL_COND_MSG(light_data->is_using_spherical_harmonics() &&
								  !light_data->_is_using_packed_directional(),
				vformat("%s (%s): The directional lightmap textures are stored in a format that "
						"isn't supported anymore. Please bake lightmaps again to make lightmaps "
						"display from this node again.",
					get_light_data()->get_path(), get_name()));

			if (last_owner && last_owner != get_owner()) {
				light_data->clear_users();
			}

			_assign_lightmaps();
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		last_owner = get_owner();

		if (light_data.is_valid()) {
			_clear_lightmaps();
		}
	} break;
	}
}

Ref<LightmapGIData> LightmapGI::get_light_data() const { return light_data; }

void LightmapGI::set_bake_quality(BakeQuality p_quality) { bake_quality = p_quality; }

LightmapGI::BakeQuality LightmapGI::get_bake_quality() const { return bake_quality; }

AABB LightmapGI::get_aabb() const { return AABB(); }

bool LightmapGI::is_using_denoiser() const { return use_denoiser; }

void LightmapGI::set_denoiser_strength(float p_denoiser_strength)
{
	denoiser_strength = p_denoiser_strength;
}

float LightmapGI::get_denoiser_strength() const { return denoiser_strength; }

void LightmapGI::set_denoiser_range(int p_denoiser_range) { denoiser_range = p_denoiser_range; }

int LightmapGI::get_denoiser_range() const { return denoiser_range; }

void LightmapGI::set_directional(bool p_enable) { directional = p_enable; }

bool LightmapGI::is_directional() const { return directional; }

LightmapGIData::ShadowmaskMode LightmapGI::get_shadowmask_mode() const { return shadowmask_mode; }

void LightmapGI::set_use_texture_for_bounces(bool p_enable) { use_texture_for_bounces = p_enable; }

bool LightmapGI::is_using_texture_for_bounces() const { return use_texture_for_bounces; }

void LightmapGI::set_interior(bool p_enable) { interior = p_enable; }

bool LightmapGI::is_interior() const { return interior; }

LightmapGI::EnvironmentMode LightmapGI::get_environment_mode() const { return environment_mode; }

void LightmapGI::set_environment_custom_sky(const Ref<Sky>& p_sky)
{
	environment_custom_sky = p_sky;
}

Ref<Sky> LightmapGI::get_environment_custom_sky() const { return environment_custom_sky; }

void LightmapGI::set_environment_custom_color(const Color& p_color)
{
	environment_custom_color = p_color;
}

Color LightmapGI::get_environment_custom_color() const { return environment_custom_color; }

void LightmapGI::set_environment_custom_energy(float p_energy)
{
	environment_custom_energy = p_energy;
}

float LightmapGI::get_environment_custom_energy() const { return environment_custom_energy; }

void LightmapGI::set_bounces(int p_bounces)
{
	ERR_FAIL_COND(p_bounces < 0 || p_bounces > 16);
	bounces = p_bounces;
}

int LightmapGI::get_bounces() const { return bounces; }

void LightmapGI::set_bounce_indirect_energy(float p_indirect_energy)
{
	ERR_FAIL_COND(p_indirect_energy < 0.0);
	bounce_indirect_energy = p_indirect_energy;
}

float LightmapGI::get_bounce_indirect_energy() const { return bounce_indirect_energy; }

void LightmapGI::set_bias(float p_bias)
{
	ERR_FAIL_COND(p_bias < 0.00001);
	bias = p_bias;
}

float LightmapGI::get_bias() const { return bias; }

void LightmapGI::set_texel_scale(float p_multiplier)
{
	ERR_FAIL_COND(p_multiplier < (0.01 - CMP_EPSILON));
	texel_scale = p_multiplier;
}

float LightmapGI::get_texel_scale() const { return texel_scale; }

void LightmapGI::set_max_texture_size(int p_size)
{
	ERR_FAIL_COND_MSG(p_size < 2048, vformat("The LightmapGI maximum texture size supplied (%d) is "
											 "too small. The minimum allowed value is 2048.",
										 p_size));
	ERR_FAIL_COND_MSG(p_size > 16384, vformat("The LightmapGI maximum texture size supplied (%d) "
											  "is too large. The maximum allowed value is 16384.",
										  p_size));
	max_texture_size = p_size;
}

int LightmapGI::get_max_texture_size() const { return max_texture_size; }

bool LightmapGI::is_supersampling_enabled() const { return supersampling_enabled; }

void LightmapGI::set_supersampling_factor(float p_factor)
{
	ERR_FAIL_COND(p_factor < 1);

	supersampling_factor = p_factor;
}

float LightmapGI::get_supersampling_factor() const { return supersampling_factor; }

void LightmapGI::set_generate_probes(GenerateProbes p_generate_probes)
{
	gen_probes = p_generate_probes;
}

LightmapGI::GenerateProbes LightmapGI::get_generate_probes() const { return gen_probes; }

void LightmapGI::set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes)
{
	camera_attributes = p_camera_attributes;
}

Ref<CameraAttributes> LightmapGI::get_camera_attributes() const { return camera_attributes; }

PackedStringArray LightmapGI::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

#ifdef MODULE_LIGHTMAPPER_RD_ENABLED
	if (!DisplayServer::can_create_rendering_device()) {
		warnings.push_back(vformat(
			RTR("Lightmaps can only be baked from a GPU that supports the RenderingDevice "
				"backends.\nYour GPU (%s) does not support RenderingDevice, as it does not support "
				"Vulkan, Direct3D 12, or Metal.\nLightmap baking will not be available on this "
				"device, although rendering existing baked lightmaps will work."),
			Renderer::get_video_adapter_name()));
		return warnings;
	}

	if (shadowmask_mode != LightmapGIData::SHADOWMASK_MODE_NONE && light_data.is_valid() &&
		!light_data->has_shadowmask_textures()) {
		warnings.push_back(RTR("The lightmap has no baked shadowmask textures. Please rebake with "
							   "the Shadowmask Mode set to anything other than None."));
	}

#elif defined(ANDROID_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
	warnings.push_back(vformat(
		RTR("Lightmaps cannot be baked on %s. Rendering existing baked lightmaps will still work."),
		OS::get_name()));
#else
	warnings.push_back(RTR("Lightmaps cannot be baked, as the `lightmapper_rd` module was disabled "
						   "at compile-time. Rendering existing baked lightmaps will still work."));
#endif

	return warnings;
}

LightmapGI::LightmapGI() {}



bool LightmapGIData::has_shadowmask_textures() { return false; }

void LightmapGI::_clear_lightmaps() {}

void LightmapGI::_assign_lightmaps() {}

// ==========================================================================
// Merged from scene/3d/lightmap_probe.cpp
// ==========================================================================


LightmapProbe::LightmapProbe() {
}

// ==========================================================================
// Merged from scene/3d/lightmapper.cpp
// ==========================================================================

#include "scene/3d/visual_instance_3d.h"

#include "modules/modules_enabled.gen.h"

Ref<LightmapDenoiser> (*LightmapDenoiser::create_function)() = nullptr;

Ref<LightmapDenoiser> LightmapDenoiser::create()
{
	if (create_function) {
		return Ref<LightmapDenoiser>(create_function());
	}
	return Ref<LightmapDenoiser>();
}

#if !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)

Ref<LightmapRaycaster> LightmapRaycaster::create() { return Ref<LightmapRaycaster>(); }

bool LightmapRaycaster::intersect(Ray& p_ray) { return false; }

void LightmapRaycaster::intersect(Vector<Ray>& r_rays) {}

void LightmapRaycaster::add_mesh(const Vector<Vector3>& p_vertices,
	const Vector<Vector3>& p_normals, const Vector<Vector2>& p_uv2s, unsigned int p_id)
{
}

void LightmapRaycaster::set_mesh_alpha_texture(Ref<Image> p_alpha_texture, unsigned int p_id) {}

void LightmapRaycaster::commit() {}

void LightmapRaycaster::set_mesh_filter(const HashSet<int>& p_mesh_ids) {}

void LightmapRaycaster::clear_mesh_filter() {}

LightmapRaycaster::LightmapRaycaster() {}

LightmapRaycaster::~LightmapRaycaster() {}

#endif // !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)

#ifndef MODULE_LIGHTMAPPER_RD_ENABLED

Ref<Lightmapper> Lightmapper::create() { return Ref<Lightmapper>(); }

Lightmapper::Lightmapper() {}

Lightmapper::~Lightmapper() {}

void Lightmapper::add_mesh(const MeshData& p_mesh) {}

void Lightmapper::add_directional_light(const String& p_name, bool p_static,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_angular_distance, float p_shadow_blur)
{
}

void Lightmapper::add_omni_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Color& p_color, float p_energy, float p_indirect_energy, float p_range,
	float p_attenuation, float p_size, float p_shadow_blur)
{
}

void Lightmapper::add_spot_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_range, float p_attenuation, float p_spot_angle, float p_spot_attenuation, float p_size,
	float p_shadow_blur)
{
}

void Lightmapper::add_area_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_range, float p_attenuation, const Vector3& p_area_width, const Vector3& p_area_height,
	float p_size, float p_shadow_blur, const Rect2& p_texture_rect, float p_max_mipmap)
{
}

void Lightmapper::add_area_light_atlas(
	const Vector2i& p_size, int p_mipmap_count, const PackedByteArray& p_atlas_data)
{
}

void Lightmapper::add_probe(const Vector3& p_position) {}

Lightmapper::BakeError Lightmapper::bake(BakeQuality p_quality, bool p_use_denoiser,
	float p_denoiser_strength, int p_denoiser_range, int p_bounces, float p_bounce_indirect_energy,
	float p_bias, int p_max_texture_size, bool p_bake_sh, bool p_bake_shadowmask,
	bool p_texture_for_bounces, GenerateProbes p_generate_probes,
	const Ref<Image>& p_environment_panorama, const Basis& p_environment_transform,
	BakeStepFunc p_step_function, void* p_step_userdata, float p_exposure_normalization,
	float p_supersampling_factor)
{
	return BAKE_ERROR_LIGHTMAP_CANT_PRE_BAKE_MESHES;
}

int Lightmapper::get_bake_texture_count() const { return 0; }

Ref<Image> Lightmapper::get_bake_texture(int p_index) const { return Ref<Image>(); }

int Lightmapper::get_shadowmask_texture_count() const { return 0; }

Ref<Image> Lightmapper::get_shadowmask_texture(int p_index) const { return Ref<Image>(); }

int Lightmapper::get_bake_mesh_count() const { return 0; }

Rect2 Lightmapper::get_bake_mesh_uv_scale(int p_index) const { return Rect2(); }

int Lightmapper::get_bake_mesh_texture_slice(int p_index) const { return 0; }

int Lightmapper::get_bake_probe_count() const { return 0; }

Vector3 Lightmapper::get_bake_probe_point(int p_probe) const { return Vector3(); }

Vector<Color> Lightmapper::get_bake_probe_sh(int p_probe) const { return Vector<Color>(); }

#endif // MODULE_LIGHTMAPPER_RD_ENABLED

// ==========================================================================
// Merged from scene/3d/reflection_probe.cpp
// ==========================================================================

#include "core/config/engine.h"
// --- Inlined from scene/3d/reflection_probe.h ---
#include "scene/3d/visual_instance_3d.h"

class ReflectionProbe : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum UpdateMode
	{
		UPDATE_ONCE,
		UPDATE_ALWAYS,
	};

	enum AmbientMode
	{
		AMBIENT_DISABLED,
		AMBIENT_ENVIRONMENT,
		AMBIENT_COLOR
	};

private:
	RID probe;
	float intensity = 1.0;
	float blend_distance = 1.0;
	float max_distance = 0.0;
	Vector3 size = Vector3(20, 20, 20);
	Vector3 origin_offset = Vector3(0, 0, 0);
	bool box_projection = false;
	bool enable_shadows = false;
	bool interior = false;
	AmbientMode ambient_mode = AMBIENT_ENVIRONMENT;
	Color ambient_color = Color(0, 0, 0);
	float ambient_color_energy = 1.0;
	float mesh_lod_threshold = 1.0;

	uint32_t cull_mask = (1 << 20) - 1;
	uint32_t reflection_mask = (1 << 20) - 1;
	UpdateMode update_mode = UPDATE_ONCE;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_intensity(float p_intensity);
	float get_intensity() const;

	float get_blend_distance() const;

	void set_ambient_mode(AmbientMode p_mode);
	AmbientMode get_ambient_mode() const;

	void set_ambient_color(Color p_ambient);
	Color get_ambient_color() const;

	void set_ambient_color_energy(float p_energy);
	float get_ambient_color_energy() const;

	void set_interior_ambient_probe_contribution(float p_contribution);
	float get_interior_ambient_probe_contribution() const;

	void set_max_distance(float p_distance);
	float get_max_distance() const;

	void set_mesh_lod_threshold(float p_pixels);
	float get_mesh_lod_threshold() const;

	Vector3 get_size() const;

	Vector3 get_origin_offset() const;

	void set_as_interior(bool p_enable);
	bool is_set_as_interior() const;

	void set_enable_box_projection(bool p_enable);
	bool is_box_projection_enabled() const;

	void set_enable_shadows(bool p_enable);
	bool are_shadows_enabled() const;

	void set_cull_mask(uint32_t p_layers);
	uint32_t get_cull_mask() const;

	void set_reflection_mask(uint32_t p_layers);
	uint32_t get_reflection_mask() const;

	void set_update_mode(UpdateMode p_mode);
	UpdateMode get_update_mode() const;

	AABB get_aabb() const;

	ReflectionProbe();
	~ReflectionProbe();
};



// --- End scene/3d/reflection_probe.h ---
#include "servers/rendering/renderer.h"

void ReflectionProbe::set_intensity(float p_intensity)
{
	intensity = p_intensity;
	Renderer::reflection_probe_set_intensity(probe, p_intensity);
}

float ReflectionProbe::get_intensity() const { return intensity; }

float ReflectionProbe::get_blend_distance() const { return blend_distance; }

ReflectionProbe::AmbientMode ReflectionProbe::get_ambient_mode() const { return ambient_mode; }

void ReflectionProbe::set_ambient_color(Color p_ambient)
{
	ambient_color = p_ambient;
	Renderer::reflection_probe_set_ambient_color(probe, p_ambient);
}

void ReflectionProbe::set_ambient_color_energy(float p_energy)
{
	ambient_color_energy = p_energy;
	Renderer::reflection_probe_set_ambient_energy(probe, p_energy);
}

float ReflectionProbe::get_ambient_color_energy() const { return ambient_color_energy; }

Color ReflectionProbe::get_ambient_color() const { return ambient_color; }

void ReflectionProbe::set_max_distance(float p_distance)
{
	max_distance = CLAMP(p_distance, 0.0, 262'144.0);
	// Reflection rendering breaks if distance exceeds 262,144 units (due to floating-point
	// precision with the near plane being 0.01).
	Renderer::reflection_probe_set_max_distance(probe, max_distance);
}

float ReflectionProbe::get_max_distance() const { return max_distance; }

void ReflectionProbe::set_mesh_lod_threshold(float p_pixels)
{
	mesh_lod_threshold = p_pixels;
	Renderer::reflection_probe_set_mesh_lod_threshold(probe, p_pixels);
}

float ReflectionProbe::get_mesh_lod_threshold() const { return mesh_lod_threshold; }

Vector3 ReflectionProbe::get_size() const { return size; }

Vector3 ReflectionProbe::get_origin_offset() const { return origin_offset; }

void ReflectionProbe::set_enable_box_projection(bool p_enable)
{
	box_projection = p_enable;
	Renderer::reflection_probe_set_enable_box_projection(probe, p_enable);
}

bool ReflectionProbe::is_box_projection_enabled() const { return box_projection; }

void ReflectionProbe::set_as_interior(bool p_enable)
{
	interior = p_enable;
	Renderer::reflection_probe_set_as_interior(probe, interior);
}

bool ReflectionProbe::is_set_as_interior() const { return interior; }

void ReflectionProbe::set_enable_shadows(bool p_enable)
{
	enable_shadows = p_enable;
	Renderer::reflection_probe_set_enable_shadows(probe, p_enable);
}

bool ReflectionProbe::are_shadows_enabled() const { return enable_shadows; }

void ReflectionProbe::set_cull_mask(uint32_t p_layers)
{
	cull_mask = p_layers;
	Renderer::reflection_probe_set_cull_mask(probe, p_layers);
}

uint32_t ReflectionProbe::get_cull_mask() const { return cull_mask; }

void ReflectionProbe::set_reflection_mask(uint32_t p_layers)
{
	reflection_mask = p_layers;
	Renderer::reflection_probe_set_reflection_mask(probe, p_layers);
}

uint32_t ReflectionProbe::get_reflection_mask() const { return reflection_mask; }

void ReflectionProbe::set_update_mode(UpdateMode p_mode)
{
	update_mode = p_mode;
	Renderer::reflection_probe_set_update_mode(
		probe, RSE::ReflectionProbeUpdateMode(p_mode));
}

ReflectionProbe::UpdateMode ReflectionProbe::get_update_mode() const { return update_mode; }

AABB ReflectionProbe::get_aabb() const
{
	AABB aabb;
	aabb.position = -size / 2;
	aabb.size = size;
	return aabb;
}

void ReflectionProbe::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void ReflectionProbe::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

ReflectionProbe::ReflectionProbe()
{
	probe = Renderer::reflection_probe_create();
	set_base(probe);
	set_disable_scale(true);
}

ReflectionProbe::~ReflectionProbe()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(probe);
}

// ==========================================================================
// Merged from scene/3d/voxel_gi.cpp
// ==========================================================================

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"
// --- Inlined from scene/3d/voxel_gi.h ---
#include "scene/3d/visual_instance_3d.h"

class CameraAttributes;
class Mesh;

class VoxelGIData : public Resource
{
	RID probe;

	Transform3D to_cell_xform;
	AABB bounds;
	Vector3 octree_size;

	float dynamic_range = 2.0;
	float energy = 1.0;
	float bias = 1.5;
	float normal_bias = 0.0;
	float propagation = 0.5;
	bool interior = false;
	bool use_two_bounces = true;

protected:

public:
	void allocate(const Transform3D& p_to_cell_xform, const AABB& p_aabb,
		const Vector3& p_octree_size, const Vector<uint8_t>& p_octree_cells,
		const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
		const Vector<int>& p_level_counts);
	AABB get_bounds() const;
	Vector3 get_octree_size() const;
	Vector<uint8_t> get_octree_cells() const;
	Vector<uint8_t> get_data_cells() const;
	Vector<uint8_t> get_distance_field() const;
	Vector<int> get_level_counts() const;
	Transform3D get_to_cell_xform() const;

	void set_dynamic_range(float p_range);
	float get_dynamic_range() const;

	void set_propagation(float p_propagation);
	float get_propagation() const;

	void set_energy(float p_energy);
	float get_energy() const;

	void set_bias(float p_bias);
	float get_bias() const;

	void set_normal_bias(float p_normal_bias);
	float get_normal_bias() const;

	void set_interior(bool p_enable);
	bool is_interior() const;

	void set_use_two_bounces(bool p_enable);
	bool is_using_two_bounces() const;

	virtual RID get_rid() const override;

	VoxelGIData();
	~VoxelGIData();
};

class VoxelGI : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum Subdiv
	{
		SUBDIV_64,
		SUBDIV_128,
		SUBDIV_256,
		SUBDIV_512,
		SUBDIV_MAX

	};

	typedef void (*BakeBeginFunc)();
	typedef bool (*BakeStepFunc)(int, const String&);
	typedef void (*BakeEndFunc)();

private:
	Ref<VoxelGIData> probe_data;

	RID voxel_gi;

	Subdiv subdiv = SUBDIV_128;
	Vector3 size = Vector3(20, 20, 20);
	Ref<CameraAttributes> camera_attributes;

	struct PlotMesh
	{
		Ref<Material> override_material;
		Vector<Ref<Material>> instance_materials;
		Ref<Mesh> mesh;
		Transform3D local_xform;
	};

	void _find_meshes(Node* p_at_node, List<PlotMesh>& plot_meshes);
	void _debug_bake();

	float _get_camera_exposure_normalization();

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	static BakeBeginFunc bake_begin_function;
	static BakeStepFunc bake_step_function;
	static BakeEndFunc bake_end_function;

	Ref<VoxelGIData> get_probe_data() const;

	Subdiv get_subdiv() const;

	Vector3 get_size() const;

	void set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes);
	Ref<CameraAttributes> get_camera_attributes() const;

	Vector3i get_estimated_cell_size() const;

	void bake(Node* p_from_node = nullptr, bool p_create_visual_debug = false);

	AABB get_aabb() const;

	PackedStringArray get_configuration_warnings() const override;

	VoxelGI();
	~VoxelGI();
};



// --- End scene/3d/voxel_gi.h ---

void VoxelGIData::allocate(const Transform3D& p_to_cell_xform, const AABB& p_aabb,
	const Vector3& p_octree_size, const Vector<uint8_t>& p_octree_cells,
	const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
	const Vector<int>& p_level_counts)
{
	Renderer::voxel_gi_allocate_data(probe, p_to_cell_xform, p_aabb, p_octree_size,
		p_octree_cells, p_data_cells, p_distance_field, p_level_counts);
	bounds = p_aabb;
	to_cell_xform = p_to_cell_xform;
	octree_size = p_octree_size;
}

AABB VoxelGIData::get_bounds() const { return bounds; }

Vector3 VoxelGIData::get_octree_size() const { return octree_size; }

Vector<uint8_t> VoxelGIData::get_octree_cells() const
{
	return Renderer::voxel_gi_get_octree_cells(probe);
}

Vector<uint8_t> VoxelGIData::get_data_cells() const
{
	return Renderer::voxel_gi_get_data_cells(probe);
}

Vector<uint8_t> VoxelGIData::get_distance_field() const
{
	return Renderer::voxel_gi_get_distance_field(probe);
}

Vector<int> VoxelGIData::get_level_counts() const
{
	return Renderer::voxel_gi_get_level_counts(probe);
}

Transform3D VoxelGIData::get_to_cell_xform() const { return to_cell_xform; }

void VoxelGIData::set_dynamic_range(float p_range)
{
	Renderer::voxel_gi_set_dynamic_range(probe, p_range);
	dynamic_range = p_range;
}

float VoxelGIData::get_dynamic_range() const { return dynamic_range; }

void VoxelGIData::set_propagation(float p_propagation)
{
	Renderer::voxel_gi_set_propagation(probe, p_propagation);
	propagation = p_propagation;
}

float VoxelGIData::get_propagation() const { return propagation; }

void VoxelGIData::set_energy(float p_energy)
{
	Renderer::voxel_gi_set_energy(probe, p_energy);
	energy = p_energy;
}

float VoxelGIData::get_energy() const { return energy; }

void VoxelGIData::set_bias(float p_bias)
{
	Renderer::voxel_gi_set_bias(probe, p_bias);
	bias = p_bias;
}

float VoxelGIData::get_bias() const { return bias; }

void VoxelGIData::set_normal_bias(float p_normal_bias)
{
	Renderer::voxel_gi_set_normal_bias(probe, p_normal_bias);
	normal_bias = p_normal_bias;
}

float VoxelGIData::get_normal_bias() const { return normal_bias; }

void VoxelGIData::set_interior(bool p_enable)
{
	Renderer::voxel_gi_set_interior(probe, p_enable);
	interior = p_enable;
}

bool VoxelGIData::is_interior() const { return interior; }

void VoxelGIData::set_use_two_bounces(bool p_enable)
{
	Renderer::voxel_gi_set_use_two_bounces(probe, p_enable);
	use_two_bounces = p_enable;
}

bool VoxelGIData::is_using_two_bounces() const { return use_two_bounces; }

RID VoxelGIData::get_rid() const { return probe; }

VoxelGIData::VoxelGIData() { probe = Renderer::voxel_gi_create(); }

VoxelGIData::~VoxelGIData()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(probe);
}

Ref<VoxelGIData> VoxelGI::get_probe_data() const { return probe_data; }

VoxelGI::Subdiv VoxelGI::get_subdiv() const { return subdiv; }

Vector3 VoxelGI::get_size() const { return size; }

void VoxelGI::set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes)
{
	camera_attributes = p_camera_attributes;

	if (probe_data.is_valid()) {
		Renderer::voxel_gi_set_baked_exposure_normalization(
			probe_data->get_rid(), _get_camera_exposure_normalization());
	}
}

Ref<CameraAttributes> VoxelGI::get_camera_attributes() const { return camera_attributes; }

VoxelGI::BakeBeginFunc VoxelGI::bake_begin_function = nullptr;
VoxelGI::BakeStepFunc VoxelGI::bake_step_function = nullptr;
VoxelGI::BakeEndFunc VoxelGI::bake_end_function = nullptr;

static int voxelizer_plot_bake_base = 0;
static int voxelizer_plot_bake_total = 0;

static bool voxelizer_plot_bake_step_function(int current, int)
{
	return VoxelGI::bake_step_function(
		(voxelizer_plot_bake_base + current) * 500 / voxelizer_plot_bake_total,
		RTR("Plotting Meshes"));
}

static bool voxelizer_sdf_bake_step_function(int current, int total)
{
	return VoxelGI::bake_step_function(
		500 + current * 500 / total, RTR("Generating Distance Field"));
}

Vector3i VoxelGI::get_estimated_cell_size() const
{
	static const int subdiv_value[SUBDIV_MAX] = {6, 7, 8, 9};
	int cell_subdiv = subdiv_value[subdiv];
	int axis_cell_size[3];
	AABB bounds = AABB(-size / 2, size);
	int longest_axis = bounds.get_longest_axis_index();
	axis_cell_size[longest_axis] = 1 << cell_subdiv;

	for (int i = 0; i < 3; i++) {
		if (i == longest_axis) {
			continue;
		}

		axis_cell_size[i] = axis_cell_size[longest_axis];
		float axis_size = bounds.size[longest_axis];

		// shrink until fit subdiv
		while (axis_size / 2.0 >= bounds.size[i]) {
			axis_size /= 2.0;
			axis_cell_size[i] >>= 1;
		}
	}

	return Vector3i(axis_cell_size[0], axis_cell_size[1], axis_cell_size[2]);
}

void VoxelGI::_debug_bake() { bake(nullptr, true); }

float VoxelGI::_get_camera_exposure_normalization()
{
	float exposure_normalization = 1.0;
	if (camera_attributes.is_valid()) {
		exposure_normalization = camera_attributes->get_exposure_multiplier();
		if (GLOBAL_GET_CACHED(bool, "rendering/lights_and_shadows/use_physical_light_units")) {
			exposure_normalization = camera_attributes->calculate_exposure_normalization();
		}
	}
	return exposure_normalization;
}

AABB VoxelGI::get_aabb() const { return AABB(-size / 2, size); }

PackedStringArray VoxelGI::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (OS::get_current_rendering_method() == "gl_compatibility") {
		warnings.push_back(RTR("VoxelGI nodes are not supported when using the Compatibility "
							   "renderer yet. Support will be added in a future release."));
	}
	else if (OS::get_current_rendering_method() == "dummy") {
		warnings.push_back(RTR("VoxelGI nodes are not supported when using the Dummy renderer."));
	}
	else if (probe_data.is_null()) {
		warnings.push_back(RTR(
			"No VoxelGI data set, so this node is disabled. Bake static objects to enable GI."));
	}
	return warnings;
}

void VoxelGI::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void VoxelGI::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

VoxelGI::VoxelGI()
{
	voxel_gi = Renderer::voxel_gi_create();
	set_disable_scale(true);
}

VoxelGI::~VoxelGI()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(voxel_gi);
}

void VoxelGI::bake(Node*, bool) {}
