/**************************************************************************/
/*  node_3d_editor_plugin.cpp                                             */
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
#include "core/input/input.h"
#include "core/math/geometry_3d.h"
#include "core/math/math_funcs.h"
#include "core/math/projection.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/plugins/editor_plugin_list.h"
#include "editor/run/editor_run_bar.h"
#include "editor/scene/3d/gizmos/audio_listener_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/audio_stream_player_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/camera_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/chain_ik_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/cpu_particles_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/decal_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/fog_volume_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/geometry_instance_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/gpu_particles_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/gpu_particles_collision_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/label_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/light_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/lightmap_gi_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/lightmap_probe_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/marker_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/mesh_instance_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/occluder_instance_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/particles_3d_emission_shape_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/collision_object_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/collision_polygon_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/collision_shape_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/joint_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/physics_bone_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/ray_cast_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/shape_cast_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/soft_body_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/spring_arm_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/physics/vehicle_body_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/reflection_probe_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/spring_bone_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/sprite_base_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/two_bone_ik_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/visible_on_screen_notifier_3d_gizmo_plugin.h"
#include "editor/scene/3d/gizmos/voxel_gi_gizmo_plugin.h"
#include "editor/scene/3d/node_3d_editor_constants.h"
#include "editor/scene/3d/node_3d_editor_gizmos.h"
#include "editor/scene/3d/node_3d_editor_viewport.h"
#include "editor/settings/editor_settings.h"
#include "editor/translations/editor_translation_preview_menu.h"
#include "node_3d_editor_plugin.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/physics_body_3d.h"
#include "scene/3d/world_environment.h"
#include "scene/gui/button.h"
#include "scene/gui/center_container.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/split_container.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/sky_material.h"
#include "scene/resources/sky.h"
#include "scene/resources/surface_tool.h"
#include "servers/physics_3d/physics_server_3d_types.h"
#include "servers/rendering/rendering_server.h"

using namespace Node3DEditorConstants;

///////////////////////////////////////////////////////////////////

Node3DEditor* Node3DEditor::singleton = nullptr;

void Node3DEditor::select_gizmo_highlight_axis(int p_axis)
{
	for (int i = 0; i < 3; i++) {
		move_gizmo[i]->surface_set_material(0, i == p_axis ? gizmo_color_hl[i] : gizmo_color[i]);
		move_plane_gizmo[i]->surface_set_material(
			0, (i + 6) == p_axis ? plane_gizmo_color_hl[i] : plane_gizmo_color[i]);
		scale_gizmo[i]->surface_set_material(
			0, (i + 9) == p_axis ? gizmo_color_hl[i] : gizmo_color[i]);
		scale_plane_gizmo[i]->surface_set_material(
			0, (i + 12) == p_axis ? plane_gizmo_color_hl[i] : plane_gizmo_color[i]);
	}

	for (int i = 0; i < 4; i++) {
		bool highlight;
		if (i == 3) {
			highlight = (p_axis == GIZMO_HIGHLIGHT_AXIS_VIEW_ROTATION);
		}
		else {
			highlight = (i + 3) == p_axis;
		}
		rotate_gizmo[i]->surface_set_material(
			0, highlight ? rotate_gizmo_color_hl[i] : rotate_gizmo_color[i]);
	}

	bool highlight_trackball = (p_axis == GIZMO_HIGHLIGHT_AXIS_TRACKBALL);
	trackball_sphere_gizmo->surface_set_material(
		0, highlight_trackball ? trackball_sphere_material_hl : trackball_sphere_material);
}

void Node3DEditor::_snap_update()
{
	snap_translate->set_value(snap_translate_value);
	snap_rotate->set_value(snap_rotate_value);
	snap_scale->set_value(snap_scale_value);
}

void Node3DEditor::_update_vertex_snap_tooltips()
{
	String snap_key = ED_GET_SHORTCUT("spatial_editor/vertex_snap")->get_as_text();
	PopupMenu* p = transform_menu->get_popup();
	p->set_item_tooltip(p->get_item_index(MENU_VERTEX_SNAP_BASE_VERTEX),
		vformat(
			TTR("Hold %s to highlight a vertex on the currently selected node,\nthen drag to move "
				"the node and snap it to vertices on neighboring nodes.\n\nFor nodes without a "
				"vertex-based representation,\nSnap Origin to Vertex is always used instead."),
			snap_key));
	p->set_item_tooltip(p->get_item_index(MENU_VERTEX_SNAP_BASE_ORIGIN),
		vformat(TTR("Hold %s to highlight another node's vertex,\nthen click to teleport the "
					"selected node to the highlighted vertex."),
			snap_key));
	p->set_item_tooltip(p->get_item_index(MENU_VERTEX_SNAP_SOURCE_MESH),
		TTR("Snap to vertices of visual meshes.\nHold Shift while vertex snapping to temporarily "
			"snap to collision shapes instead."));
	p->set_item_tooltip(p->get_item_index(MENU_VERTEX_SNAP_SOURCE_COLLISION),
		TTR("Snap to vertices of collision shapes.\nHold Shift while vertex snapping to "
			"temporarily snap to mesh vertices instead."));
}

void Node3DEditor::_finish_indicators()
{
	RenderingServer::get_singleton()->free_rid(origin_instance);
	RenderingServer::get_singleton()->free_rid(origin_multimesh);
	RenderingServer::get_singleton()->free_rid(origin_mesh);

	_finish_grid();
}

void Node3DEditor::_finish_grid()
{
	for (int i = 0; i < 3; i++) {
		RenderingServer::get_singleton()->free_rid(grid_instance[i]);
		RenderingServer::get_singleton()->free_rid(grid[i]);
	}
}

void Node3DEditor::update_grid()
{
	const Camera3D::ProjectionType current_projection = viewports[0]->camera->get_projection();

	if (current_projection != grid_camera_last_update_perspective) {
		grid_init_draw = false; // redraw
		grid_camera_last_update_perspective = current_projection;
	}

	// Gets a orthogonal or perspective position correctly (for the grid comparison)
	const Vector3 camera_position = get_editor_viewport(0)->camera->get_position();

	if (!grid_init_draw ||
		grid_camera_last_update_position.distance_squared_to(camera_position) >= 100.0f) {
		_finish_grid();
		_init_grid();
		grid_init_draw = true;
		grid_camera_last_update_position = camera_position;
	}
}

void Node3DEditor::snap_selected_nodes_to_floor() { do_snap_selected_nodes_to_floor = true; }

void Node3DEditor::_sun_environ_settings_pressed()
{
	Vector2 pos = sun_environ_settings->get_screen_position() + sun_environ_settings->get_size();
	sun_environ_popup->set_position(
		pos - Vector2(sun_environ_popup->get_contents_minimum_size().width / 2, 0));
	sun_environ_popup->reset_size();
	sun_environ_popup->popup();
	// Grabbing the focus is required for Shift modifier checking to be functional
	// (when the Add sun/environment buttons are pressed).
	sun_environ_popup->grab_focus();
}

void Node3DEditor::set_can_preview(Camera3D* p_preview)
{
	for (int i = 0; i < 4; i++) {
		viewports[i]->set_can_preview(p_preview);
	}

	viewports[last_used_viewport]->switch_preview_camera(p_preview);
}

VSplitContainer* Node3DEditor::get_shader_split() { return shader_split; }

Node3DEditorViewport* Node3DEditor::get_last_used_viewport()
{
	return viewports[last_used_viewport];
}

void Node3DEditor::set_freelook_viewport(Node3DEditorViewport* p_viewport)
{
	freelook_viewport = p_viewport;
}

Node3DEditorViewport* Node3DEditor::get_freelook_viewport() const { return freelook_viewport; }

void Node3DEditor::_viewport_clicked(int p_viewport_idx) { last_used_viewport = p_viewport_idx; }

void Node3DEditor::_register_all_gizmos()
{
	add_gizmo_plugin(Ref<Camera3DGizmoPlugin>(memnew(Camera3DGizmoPlugin)));
	add_gizmo_plugin(Ref<Light3DGizmoPlugin>(memnew(Light3DGizmoPlugin)));
	add_gizmo_plugin(Ref<AudioStreamPlayer3DGizmoPlugin>(memnew(AudioStreamPlayer3DGizmoPlugin)));
	add_gizmo_plugin(Ref<AudioListener3DGizmoPlugin>(memnew(AudioListener3DGizmoPlugin)));
	add_gizmo_plugin(Ref<MeshInstance3DGizmoPlugin>(memnew(MeshInstance3DGizmoPlugin)));
	add_gizmo_plugin(Ref<OccluderInstance3DGizmoPlugin>(memnew(OccluderInstance3DGizmoPlugin)));
	add_gizmo_plugin(Ref<SpriteBase3DGizmoPlugin>(memnew(SpriteBase3DGizmoPlugin)));
	add_gizmo_plugin(Ref<Label3DGizmoPlugin>(memnew(Label3DGizmoPlugin)));
	add_gizmo_plugin(Ref<GeometryInstance3DGizmoPlugin>(memnew(GeometryInstance3DGizmoPlugin)));
	add_gizmo_plugin(Ref<Marker3DGizmoPlugin>(memnew(Marker3DGizmoPlugin)));
	add_gizmo_plugin(
		Ref<SpringBoneCollision3DGizmoPlugin>(memnew(SpringBoneCollision3DGizmoPlugin)));
	add_gizmo_plugin(
		Ref<SpringBoneSimulator3DGizmoPlugin>(memnew(SpringBoneSimulator3DGizmoPlugin)));
	add_gizmo_plugin(
		Ref<VisibleOnScreenNotifier3DGizmoPlugin>(memnew(VisibleOnScreenNotifier3DGizmoPlugin)));
	add_gizmo_plugin(Ref<GPUParticles3DGizmoPlugin>(memnew(GPUParticles3DGizmoPlugin)));
	add_gizmo_plugin(
		Ref<GPUParticlesCollision3DGizmoPlugin>(memnew(GPUParticlesCollision3DGizmoPlugin)));
	add_gizmo_plugin(
		Ref<Particles3DEmissionShapeGizmoPlugin>(memnew(Particles3DEmissionShapeGizmoPlugin)));
	add_gizmo_plugin(Ref<CPUParticles3DGizmoPlugin>(memnew(CPUParticles3DGizmoPlugin)));
	add_gizmo_plugin(Ref<ReflectionProbeGizmoPlugin>(memnew(ReflectionProbeGizmoPlugin)));
	add_gizmo_plugin(Ref<DecalGizmoPlugin>(memnew(DecalGizmoPlugin)));
	add_gizmo_plugin(Ref<VoxelGIGizmoPlugin>(memnew(VoxelGIGizmoPlugin)));
	add_gizmo_plugin(Ref<LightmapGIGizmoPlugin>(memnew(LightmapGIGizmoPlugin)));
	add_gizmo_plugin(Ref<LightmapProbeGizmoPlugin>(memnew(LightmapProbeGizmoPlugin)));
	add_gizmo_plugin(Ref<FogVolumeGizmoPlugin>(memnew(FogVolumeGizmoPlugin)));
	add_gizmo_plugin(Ref<TwoBoneIK3DGizmoPlugin>(memnew(TwoBoneIK3DGizmoPlugin)));
	add_gizmo_plugin(Ref<ChainIK3DGizmoPlugin>(memnew(ChainIK3DGizmoPlugin)));
	// Physics gizmo plugins.
	add_gizmo_plugin(Ref<CollisionObject3DGizmoPlugin>(memnew(CollisionObject3DGizmoPlugin)));
	add_gizmo_plugin(Ref<CollisionShape3DGizmoPlugin>(memnew(CollisionShape3DGizmoPlugin)));
	add_gizmo_plugin(Ref<CollisionPolygon3DGizmoPlugin>(memnew(CollisionPolygon3DGizmoPlugin)));
	add_gizmo_plugin(Ref<Joint3DGizmoPlugin>(memnew(Joint3DGizmoPlugin)));
	add_gizmo_plugin(Ref<SoftBody3DGizmoPlugin>(memnew(SoftBody3DGizmoPlugin)));
	add_gizmo_plugin(Ref<ShapeCast3DGizmoPlugin>(memnew(ShapeCast3DGizmoPlugin)));
	add_gizmo_plugin(Ref<SpringArm3DGizmoPlugin>(memnew(SpringArm3DGizmoPlugin)));
	add_gizmo_plugin(Ref<PhysicalBone3DGizmoPlugin>(memnew(PhysicalBone3DGizmoPlugin)));
	add_gizmo_plugin(Ref<VehicleWheel3DGizmoPlugin>(memnew(VehicleWheel3DGizmoPlugin)));
	add_gizmo_plugin(Ref<RayCast3DGizmoPlugin>(memnew(RayCast3DGizmoPlugin)));
}

void Node3DEditor::_preview_settings_changed()
{
	if (sun_environ_updating) {
		return;
	}

	{ // preview sun
		sun_rotation.x = Math::deg_to_rad(-sun_angle_altitude->get_value());
		sun_rotation.y = Math::deg_to_rad(180.0 - sun_angle_azimuth->get_value());
		Transform3D t;
		t.basis = Basis::from_euler(Vector3(sun_rotation.x, sun_rotation.y, 0));
		preview_sun->set_transform(t);
		sun_direction->queue_redraw();
		preview_sun->set_param(Light3D::PARAM_ENERGY, sun_energy->get_value());
		preview_sun->set_param(
			Light3D::PARAM_SHADOW_MAX_DISTANCE, sun_shadow_max_distance->get_value());
		preview_sun->set_color(sun_color->get_pick_color());
	}

	{ // preview env
		sky_material->set_energy_multiplier(environ_energy->get_value());
		Color hz_color =
			environ_sky_color->get_pick_color().lerp(environ_ground_color->get_pick_color(), 0.5);
		float hz_lum = hz_color.get_luminance() * 3.333;
		hz_color = hz_color.lerp(Color(hz_lum, hz_lum, hz_lum), 0.5);
		sky_material->set_sky_top_color(environ_sky_color->get_pick_color());
		sky_material->set_sky_horizon_color(hz_color);
		sky_material->set_ground_bottom_color(environ_ground_color->get_pick_color());
		sky_material->set_ground_horizon_color(hz_color);

		environment->set_ssao_enabled(environ_ao_button->is_pressed());
		environment->set_glow_enabled(environ_glow_button->is_pressed());
		environment->set_sdfgi_enabled(environ_gi_button->is_pressed());
		environment->set_tonemapper(environ_tonemap_button->is_pressed()
										? Environment::TONE_MAPPER_FILMIC
										: Environment::TONE_MAPPER_LINEAR);
	}
}

void Node3DEditor::_load_default_preview_settings()
{
	sun_environ_updating = true;

	// These default rotations place the preview sun at an angular altitude
	// of 60 degrees (must be negative) and an azimuth of 30 degrees clockwise
	// from north (or 150 CCW from south), from north east, facing south west.
	// On any not-tidally-locked planet, a sun would have an angular altitude
	// of 60 degrees as the average of all points on the sphere at noon.
	// The azimuth choice is arbitrary, but ideally shouldn't be on an axis.
	sun_rotation = Vector2(-Math::deg_to_rad(60.0), Math::deg_to_rad(150.0));

	sun_angle_altitude->set_value_no_signal(-Math::rad_to_deg(sun_rotation.x));
	sun_angle_azimuth->set_value_no_signal(180.0 - Math::rad_to_deg(sun_rotation.y));
	sun_direction->queue_redraw();
	environ_sky_color->set_pick_color(Color(0.385, 0.454, 0.55));
	environ_ground_color->set_pick_color(Color(0.2, 0.169, 0.133));
	environ_energy->set_value_no_signal(1.0);
	if (OS::get_singleton()->get_current_rendering_method() != "gl_compatibility" &&
		OS::get_singleton()->get_current_rendering_method() != "dummy") {
		environ_glow_button->set_pressed_no_signal(true);
	}
	environ_tonemap_button->set_pressed_no_signal(false);
	environ_ao_button->set_pressed_no_signal(false);
	environ_gi_button->set_pressed_no_signal(false);
	sun_shadow_max_distance->set_value_no_signal(100);

	sun_color->set_pick_color(Color(1, 1, 1));
	sun_energy->set_value_no_signal(1.0);

	sun_environ_updating = false;
}

Node3DEditor::~Node3DEditor()
{
	singleton = nullptr;
	memdelete(preview_node);
	if (preview_sun_dangling && preview_sun) {
		memdelete(preview_sun);
	}
	if (preview_env_dangling && preview_environment) {
		memdelete(preview_environment);
	}
}

Vector3 Node3DEditor::snap_point(Vector3 p_target, Vector3 p_start) const
{
	if (is_snap_enabled()) {
		real_t snap = get_translate_snap();
		p_target.snapf(snap);
	}
	return p_target;
}

float Node3DEditor::get_znear() const { return settings_znear->get_value(); }

float Node3DEditor::get_zfar() const { return settings_zfar->get_value(); }

float Node3DEditor::get_fov() const { return settings_fov->get_value(); }

bool Node3DEditor::is_gizmo_visible() const
{
	if (selected) {
		return gizmo.visible && selected->is_transform_gizmo_visible();
	}
	return gizmo.visible;
}

bool Node3DEditor::are_local_coords_enabled() const
{
	return tool_option_button[Node3DEditor::TOOL_OPT_LOCAL_COORDS]->is_pressed();
}

void Node3DEditor::set_local_coords_enabled(bool on) const
{
	tool_option_button[Node3DEditor::TOOL_OPT_LOCAL_COORDS]->set_pressed(on);
}

bool Node3DEditor::is_preserve_children_transform_enabled() const
{
	return tool_option_button[Node3DEditor::TOOL_OPT_PRESERVE_CHILDREN_TRANSFORM]->is_pressed();
}

bool Node3DEditor::is_vertex_snap_use_collision() const
{
	return vertex_snap_use_collision != Input::get_singleton()->is_key_pressed(Key::SHIFT);
}

real_t Node3DEditor::get_translate_snap() const
{
	real_t snap_value = snap_translate_value;
	if (Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
		snap_value /= 10.0f;
	}
	return snap_value;
}

real_t Node3DEditor::get_rotate_snap() const
{
	real_t snap_value = snap_rotate_value;
	if (Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
		snap_value /= 3.0f;
	}
	return snap_value;
}

real_t Node3DEditor::get_scale_snap() const
{
	real_t snap_value = snap_scale_value;
	if (Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
		snap_value /= 2.0f;
	}
	return snap_value;
}

struct _GizmoPluginPriorityComparator
{
	bool operator()(
		const Ref<EditorNode3DGizmoPlugin>& p_a, const Ref<EditorNode3DGizmoPlugin>& p_b) const
	{
		if (p_a->get_priority() == p_b->get_priority()) {
			return p_a->get_gizmo_name() < p_b->get_gizmo_name();
		}
		return p_a->get_priority() > p_b->get_priority();
	}
};

struct _GizmoPluginNameComparator
{
	bool operator()(
		const Ref<EditorNode3DGizmoPlugin>& p_a, const Ref<EditorNode3DGizmoPlugin>& p_b) const
	{
		return p_a->get_gizmo_name() < p_b->get_gizmo_name();
	}
};

void Node3DEditor::add_gizmo_plugin(Ref<EditorNode3DGizmoPlugin> p_plugin)
{
	ERR_FAIL_COND(p_plugin.is_null());

	gizmo_plugins_by_priority.push_back(p_plugin);
	gizmo_plugins_by_priority.sort_custom<_GizmoPluginPriorityComparator>();

	gizmo_plugins_by_name.push_back(p_plugin);
	gizmo_plugins_by_name.sort_custom<_GizmoPluginNameComparator>();

	_update_gizmos_menu();
}

void Node3DEditor::remove_gizmo_plugin(Ref<EditorNode3DGizmoPlugin> p_plugin)
{
	gizmo_plugins_by_priority.erase(p_plugin);
	gizmo_plugins_by_name.erase(p_plugin);
	_update_gizmos_menu();
}

DynamicBVH::ID Node3DEditor::insert_gizmo_bvh_node(Node3D* p_node, const AABB& p_aabb)
{
	return gizmo_bvh.insert(p_aabb, p_node);
}

void Node3DEditor::update_gizmo_bvh_node(DynamicBVH::ID p_id, const AABB& p_aabb)
{
	gizmo_bvh.update(p_id, p_aabb);
	gizmo_bvh.optimize_incremental(1);
}

void Node3DEditor::remove_gizmo_bvh_node(DynamicBVH::ID p_id) { gizmo_bvh.remove(p_id); }

Vector<Node3D*> Node3DEditor::gizmo_bvh_ray_query(
	const Vector3& p_ray_start, const Vector3& p_ray_end)
{
	struct Result
	{
		Vector<Node3D*> nodes;

		bool operator()(void* p_data)
		{
			nodes.append((Node3D*)p_data);
			return false;
		}
	} result;

	gizmo_bvh.ray_query(p_ray_start, p_ray_end, result);

	return result.nodes;
}

Vector<Node3D*> Node3DEditor::gizmo_bvh_frustum_query(const Vector<Plane>& p_frustum)
{
	Vector<Vector3> points =
		Geometry3D::compute_convex_mesh_points(&p_frustum[0], p_frustum.size());

	struct Result
	{
		Vector<Node3D*> nodes;

		bool operator()(void* p_data)
		{
			nodes.append((Node3D*)p_data);
			return false;
		}
	};

	Result result;

	gizmo_bvh.convex_query(p_frustum.ptr(), p_frustum.size(), points.ptr(), points.size(), result);

	return result.nodes;
}


