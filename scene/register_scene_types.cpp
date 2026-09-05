/**************************************************************************/
/*  register_scene_types.cpp                                              */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/os.h"
#include "register_scene_types.h"
#include "scene/animation/animation_blend_space_1d.h"
#include "scene/animation/animation_blend_space_2d.h"
#include "scene/animation/animation_blend_tree.h"
#include "scene/animation/animation_mixer.h"
#include "scene/animation/animation_node_extension.h"
#include "scene/animation/animation_node_state_machine.h"
#include "scene/animation/animation_player.h"
#include "scene/animation/animation_tree.h"
#include "scene/animation/tween.h"
#include "scene/audio/audio_stream_player.h"
#include "scene/debugger/scene_debugger.h"
#include "scene/gui/aspect_ratio_container.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/center_container.h"
#include "scene/gui/check_box.h"
#include "scene/gui/check_button.h"
#include "scene/gui/code_edit.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/color_picker_shape.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/control.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/foldable_container.h"
#include "scene/gui/graph_edit.h"
#include "scene/gui/graph_frame.h"
#include "scene/gui/graph_node.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/link_button.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/menu_bar.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/nine_patch_rect.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/reference_rect.h"
#include "scene/gui/rich_text_effect.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/scroll_bar.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/split_container.h"
#include "scene/gui/subviewport_container.h"
#include "scene/gui/tab_bar.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/texture_button.h"
#include "scene/gui/texture_progress_bar.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"
#include "scene/gui/video_stream_player.h"
#include "scene/gui/virtual_joystick.h"
#include "scene/main/canvas_item.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/http_request.h"
#include "scene/main/instance_placeholder.h"
#include "scene/main/missing_node.h"
#include "scene/main/multiplayer_api.h"
#include "scene/main/resource_preloader.h"
#include "scene/main/scene_tree.h"
#include "scene/main/shader_globals_override.h"
#include "scene/main/status_indicator.h"
#include "scene/main/timer.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "scene/resources/animation_library.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/audio_stream_polyphonic.h"
#include "scene/resources/audio_stream_wav.h"
#include "scene/resources/bit_map.h"
#include "scene/resources/blit_material.h"
#include "scene/resources/bone_map.h"
#include "scene/resources/camera_attributes.h"
#include "scene/resources/camera_texture.h"
#include "scene/resources/canvas_item_material.h"
#include "scene/resources/color_palette.h"
#include "scene/resources/compositor.h"
#include "scene/resources/compressed_texture.h"
#include "scene/resources/compressed_texture_resource_format.h"
#include "scene/resources/curve_texture.h"
#include "scene/resources/drawable_texture_2d.h"
#include "scene/resources/environment.h"
#include "scene/resources/external_texture.h"
#include "scene/resources/font.h"
#include "scene/resources/gradient.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/immediate_mesh.h"
#include "scene/resources/label_settings.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh_data_tool.h"
#include "scene/resources/mesh_texture.h"
#include "scene/resources/multimesh.h"
#if !defined(NAVIGATION_2D_DISABLED) || !defined(NAVIGATION_3D_DISABLED)
#include "scene/resources/navigation_mesh.h"
#endif // !defined(NAVIGATION_2D_DISABLED) || !defined(NAVIGATION_3D_DISABLED)
#include "scene/resources/dpi_texture.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/particle_process_material.h"
#include "scene/resources/placeholder_textures.h"
#include "scene/resources/portable_compressed_texture.h"
#include "scene/resources/resource_format_text.h"
#include "scene/resources/shader_include.h"
#include "scene/resources/shader_include_resource_format.h"
#include "scene/resources/shader_resource_format.h"
#include "scene/resources/skeleton_profile.h"
#include "scene/resources/sky.h"
#include "scene/resources/style_box.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/style_box_line.h"
#include "scene/resources/style_box_texture.h"
#include "scene/resources/surface_tool.h"
#include "scene/resources/syntax_highlighter.h"
#include "scene/resources/text_line.h"
#include "scene/resources/text_paragraph.h"
#include "scene/resources/texture.h"
#include "scene/resources/texture_rd.h"
#include "scene/resources/theme.h"
#include "scene/resources/video_stream.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"

#ifndef DISABLE_DEPRECATED
#include "scene/resources/animated_texture.h"
#endif

// 2D
#include "scene/2d/animated_sprite_2d.h"
#include "scene/2d/audio_listener_2d.h"
#include "scene/2d/audio_stream_player_2d.h"
#include "scene/2d/back_buffer_copy.h"
#include "scene/2d/camera_2d.h"
#include "scene/2d/canvas_group.h"
#include "scene/2d/canvas_modulate.h"
#include "scene/2d/cpu_particles_2d.h"
#include "scene/2d/gpu_particles_2d.h"
#include "scene/2d/light_2d.h"
#include "scene/2d/light_occluder_2d.h"
#include "scene/2d/line_2d.h"
#include "scene/2d/marker_2d.h"
#include "scene/2d/mesh_instance_2d.h"
#include "scene/2d/multimesh_instance_2d.h"
#include "scene/2d/parallax_2d.h"
#include "scene/2d/path_2d.h"
#include "scene/2d/polygon_2d.h"
#include "scene/2d/remote_transform_2d.h"
#include "scene/2d/skeleton_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/2d/tile_map_layer.h"
#include "scene/2d/visible_on_screen_notifier_2d.h"
#include "scene/resources/2d/polygon_path_finder.h"
#include "scene/resources/2d/skeleton/skeleton_modification_2d.h"
#include "scene/resources/2d/skeleton/skeleton_modification_2d_ccdik.h"
#include "scene/resources/2d/skeleton/skeleton_modification_2d_fabrik.h"
#include "scene/resources/2d/skeleton/skeleton_modification_2d_lookat.h"
#include "scene/resources/2d/skeleton/skeleton_modification_2d_stackholder.h"
#include "scene/resources/2d/skeleton/skeleton_modification_2d_twoboneik.h"
#include "scene/resources/2d/skeleton/skeleton_modification_stack_2d.h"
#include "scene/resources/2d/tile_set.h"
#include "scene/resources/world_2d.h"
#ifndef DISABLE_DEPRECATED
#include "scene/2d/parallax_background.h"
#include "scene/2d/parallax_layer.h"
#include "scene/2d/tile_map.h"
#endif

#ifndef NAVIGATION_2D_DISABLED
#include "scene/2d/navigation/navigation_agent_2d.h"
#include "scene/2d/navigation/navigation_link_2d.h"
#include "scene/2d/navigation/navigation_obstacle_2d.h"
#include "scene/2d/navigation/navigation_region_2d.h"
#include "scene/resources/2d/navigation_mesh_source_geometry_data_2d.h"
#include "scene/resources/2d/navigation_polygon.h"
#endif // NAVIGATION_2D_DISABLED

#ifndef _3D_DISABLED
#include "scene/3d/aim_modifier_3d.h"
#include "scene/3d/audio_listener_3d.h"
#include "scene/3d/audio_stream_player_3d.h"
#include "scene/3d/bone_attachment_3d.h"
#include "scene/3d/bone_constraint_3d.h"
#include "scene/3d/bone_twist_disperser_3d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/ccd_ik_3d.h"
#include "scene/3d/chain_ik_3d.h"
#include "scene/3d/convert_transform_modifier_3d.h"
#include "scene/3d/copy_transform_modifier_3d.h"
#include "scene/3d/cpu_particles_3d.h"
#include "scene/3d/decal.h"
#include "scene/3d/fabr_ik_3d.h"
#include "scene/3d/fog_volume.h"
#include "scene/3d/gpu_particles_3d.h"
#include "scene/3d/gpu_particles_collision_3d.h"
#include "scene/3d/ik_modifier_3d.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/iterate_ik_3d.h"
#include "scene/3d/jacobian_ik_3d.h"
#include "scene/3d/label_3d.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/lightmap_gi.h"
#include "scene/3d/lightmap_probe.h"
#include "scene/3d/limit_angular_velocity_modifier_3d.h"
#include "scene/3d/look_at_modifier_3d.h"
#include "scene/3d/marker_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/modifier_bone_target_3d.h"
#include "scene/3d/multimesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/occluder_instance_3d.h"
#include "scene/3d/path_3d.h"
#include "scene/3d/reflection_probe.h"
#include "scene/3d/remote_transform_3d.h"
#include "scene/3d/retarget_modifier_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/skeleton_modifier_3d.h"
#include "scene/3d/spline_ik_3d.h"
#include "scene/3d/spring_bone_collision_3d.h"
#include "scene/3d/spring_bone_collision_capsule_3d.h"
#include "scene/3d/spring_bone_collision_plane_3d.h"
#include "scene/3d/spring_bone_collision_sphere_3d.h"
#include "scene/3d/spring_bone_simulator_3d.h"
#include "scene/3d/sprite_3d.h"
#include "scene/3d/two_bone_ik_3d.h"
#include "scene/3d/visible_on_screen_notifier_3d.h"
#include "scene/3d/voxel_gi.h"
#include "scene/3d/world_environment.h"
#include "scene/animation/root_motion_view.h"
#include "scene/resources/3d/fog_material.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/3d/joint_limitation_3d.h"
#include "scene/resources/3d/joint_limitation_cone_3d.h"
#include "scene/resources/3d/mesh_library.h"
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/3d/sky_material.h"
#include "scene/resources/3d/world_3d.h"
#ifndef NAVIGATION_3D_DISABLED
#include "scene/3d/navigation/navigation_agent_3d.h"
#include "scene/3d/navigation/navigation_link_3d.h"
#include "scene/3d/navigation/navigation_obstacle_3d.h"
#include "scene/3d/navigation/navigation_region_3d.h"
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"
#endif // NAVIGATION_3D_DISABLED
#ifndef XR_DISABLED
#include "scene/3d/xr/xr_body_modifier_3d.h"
#include "scene/3d/xr/xr_face_modifier_3d.h"
#include "scene/3d/xr/xr_hand_modifier_3d.h"
#include "scene/3d/xr/xr_nodes.h"
#endif // XR_DISABLED
#ifndef DISABLE_DEPRECATED
#include "scene/3d/skeleton_ik_3d.h"
#endif
#endif // _3D_DISABLED

#if !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)
#include "scene/resources/physics_material.h"
#endif // !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)

#ifndef PHYSICS_2D_DISABLED
#include "scene/2d/physics/animatable_body_2d.h"
#include "scene/2d/physics/area_2d.h"
#include "scene/2d/physics/character_body_2d.h"
#include "scene/2d/physics/collision_polygon_2d.h"
#include "scene/2d/physics/collision_shape_2d.h"
#include "scene/2d/physics/joints/damped_spring_joint_2d.h"
#include "scene/2d/physics/joints/groove_joint_2d.h"
#include "scene/2d/physics/joints/joint_2d.h"
#include "scene/2d/physics/joints/pin_joint_2d.h"
#include "scene/2d/physics/kinematic_collision_2d.h"
#include "scene/2d/physics/physical_bone_2d.h"
#include "scene/2d/physics/physics_body_2d.h"
#include "scene/2d/physics/ray_cast_2d.h"
#include "scene/2d/physics/rigid_body_2d.h"
#include "scene/2d/physics/shape_cast_2d.h"
#include "scene/2d/physics/static_body_2d.h"
#include "scene/2d/physics/touch_screen_button.h"
#include "scene/resources/2d/capsule_shape_2d.h"
#include "scene/resources/2d/circle_shape_2d.h"
#include "scene/resources/2d/concave_polygon_shape_2d.h"
#include "scene/resources/2d/convex_polygon_shape_2d.h"
#include "scene/resources/2d/rectangle_shape_2d.h"
#include "scene/resources/2d/segment_shape_2d.h"
#include "scene/resources/2d/separation_ray_shape_2d.h"
#include "scene/resources/2d/shape_2d.h"
#include "scene/resources/2d/skeleton/skeleton_modification_2d_jiggle.h"
#include "scene/resources/2d/skeleton/skeleton_modification_2d_physicalbones.h"
#include "scene/resources/2d/world_boundary_shape_2d.h"
#endif // PHYSICS_2D_DISABLED

#ifndef PHYSICS_3D_DISABLED
#include "scene/3d/physics/animatable_body_3d.h"
#include "scene/3d/physics/area_3d.h"
#include "scene/3d/physics/character_body_3d.h"
#include "scene/3d/physics/collision_polygon_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/joints/cone_twist_joint_3d.h"
#include "scene/3d/physics/joints/generic_6dof_joint_3d.h"
#include "scene/3d/physics/joints/hinge_joint_3d.h"
#include "scene/3d/physics/joints/joint_3d.h"
#include "scene/3d/physics/joints/pin_joint_3d.h"
#include "scene/3d/physics/joints/slider_joint_3d.h"
#include "scene/3d/physics/kinematic_collision_3d.h"
#include "scene/3d/physics/physical_bone_3d.h"
#include "scene/3d/physics/physical_bone_simulator_3d.h"
#include "scene/3d/physics/physics_body_3d.h"
#include "scene/3d/physics/ray_cast_3d.h"
#include "scene/3d/physics/rigid_body_3d.h"
#include "scene/3d/physics/shape_cast_3d.h"
#include "scene/3d/physics/soft_body_3d.h"
#include "scene/3d/physics/spring_arm_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/3d/physics/vehicle_body_3d.h"
#include "scene/resources/3d/box_shape_3d.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/3d/concave_polygon_shape_3d.h"
#include "scene/resources/3d/convex_polygon_shape_3d.h"
#include "scene/resources/3d/cylinder_shape_3d.h"
#include "scene/resources/3d/height_map_shape_3d.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/3d/mesh_library.h"
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/3d/separation_ray_shape_3d.h"
#include "scene/resources/3d/sphere_shape_3d.h"
#include "scene/resources/3d/world_boundary_shape_3d.h"
#endif // PHYSICS_3D_DISABLED

static Ref<ResourceFormatSaverText> resource_saver_text;
static Ref<ResourceFormatLoaderText> resource_loader_text;

static Ref<ResourceFormatLoaderCompressedTexture2D> resource_loader_stream_texture;
static Ref<ResourceFormatLoaderCompressedTextureLayered> resource_loader_texture_layered;
static Ref<ResourceFormatLoaderCompressedTexture3D> resource_loader_texture_3d;

static Ref<ResourceFormatSaverShader> resource_saver_shader;
static Ref<ResourceFormatLoaderShader> resource_loader_shader;

static Ref<ResourceFormatSaverShaderInclude> resource_saver_shader_include;
static Ref<ResourceFormatLoaderShaderInclude> resource_loader_shader_include;

void register_scene_types()
{
	OS::get_singleton()->benchmark_begin_measure("Scene", "Register Types");

	SceneStringNames::create();

	OS::get_singleton()->yield(); // may take time to init

	Node::init_node_hrcr();

	resource_loader_stream_texture.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_stream_texture);

	resource_loader_texture_layered.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_texture_layered);

	resource_loader_texture_3d.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_texture_3d);

	resource_saver_text.instantiate();
	ResourceSaver::add_resource_format_saver(resource_saver_text, true);

	resource_loader_text.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_text, true);

	resource_saver_shader.instantiate();
	ResourceSaver::add_resource_format_saver(resource_saver_shader, true);

	resource_loader_shader.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_shader, true);

	resource_saver_shader_include.instantiate();
	ResourceSaver::add_resource_format_saver(resource_saver_shader_include, true);

	resource_loader_shader_include.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_shader_include, true);

	OS::get_singleton()->yield(); // may take time to init

	/* REGISTER GUI */

	OS::get_singleton()->yield(); // may take time to init

	OS::get_singleton()->yield(); // may take time to init

	OS::get_singleton()->yield(); // may take time to init

#ifndef ADVANCED_GUI_DISABLED

	OS::get_singleton()->yield(); // may take time to init

	int swap_cancel_ok = GLOBAL_DEF(PropertyInfo(Variant::INT, "gui/common/swap_cancel_ok",
										PROPERTY_HINT_ENUM, "Auto,Cancel First,OK First"),
		0);
	if (DisplayServer::get_singleton() && swap_cancel_ok == 0) {
		swap_cancel_ok = DisplayServer::get_singleton()->get_swap_cancel_ok() ? 2 : 1;
	}
	AcceptDialog::set_swap_cancel_ok(swap_cancel_ok == 2);
#endif

	int root_dir = GLOBAL_GET("internationalization/rendering/root_node_layout_direction");
	Control::set_root_layout_direction(root_dir);
	Window::set_root_layout_direction(root_dir);

	/* REGISTER ANIMATION */

	OS::get_singleton()->yield(); // may take time to init

	/* REGISTER 3D */

#ifndef _3D_DISABLED
#ifndef DISABLE_DEPRECATED
	MeshInstance3D::use_parent_skeleton_compat =
		GLOBAL_GET("animation/compatibility/default_parent_skeleton_in_mesh_instance_3d");
#endif

	OS::get_singleton()->yield(); // may take time to init

#ifndef NAVIGATION_3D_DISABLED
#endif // NAVIGATION_3D_DISABLED

	OS::get_singleton()->yield(); // may take time to init
#endif							  // _3D_DISABLED

	/* REGISTER SHADER */

	SceneTree::add_idle_callback(CanvasItemMaterial::flush_changes);
	CanvasItemMaterial::init_shaders();

	/* REGISTER 2D */

#ifndef PHYSICS_2D_DISABLED
#endif // PHYSICS_2D_DISABLED

	OS::get_singleton()->yield(); // may take time to init

#ifndef PHYSICS_2D_DISABLED
#endif // PHYSICS_2D_DISABLED

	OS::get_singleton()->yield(); // may take time to init

	/* REGISTER RESOURCES */

	SceneTree::add_idle_callback(ParticleProcessMaterial::flush_changes);
	ParticleProcessMaterial::init_shaders();

#ifndef _3D_DISABLED
	SceneTree::add_idle_callback(BaseMaterial3D::flush_changes);
	BaseMaterial3D::init_shaders();

	OS::get_singleton()->yield(); // may take time to init

#ifndef PHYSICS_3D_DISABLED
#endif // PHYSICS_3D_DISABLED

	OS::get_singleton()->yield(); // may take time to init
#endif							  // _3D_DISABLED

#if !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)
#endif // !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)
#ifndef DISABLE_DEPRECATED
#endif

	// These classes are part of renderer_rd

	OS::get_singleton()->yield(); // may take time to init

	OS::get_singleton()->yield(); // may take time to init

#ifndef PHYSICS_2D_DISABLED
#endif // PHYSICS_2D_DISABLED

#if !defined(NAVIGATION_2D_DISABLED) || !defined(NAVIGATION_3D_DISABLED)
#endif // !defined(NAVIGATION_2D_DISABLED) || !defined(NAVIGATION_3D_DISABLED)

#ifndef NAVIGATION_2D_DISABLED

	OS::get_singleton()->yield(); // may take time to init

	// 2D nodes that support navmesh baking need to server register their source geometry parsers.
	MeshInstance2D::navmesh_parse_init();
	MultiMeshInstance2D::navmesh_parse_init();
	NavigationObstacle2D::navmesh_parse_init();
	Polygon2D::navmesh_parse_init();
#ifndef DISABLE_DEPRECATED
	TileMap::navmesh_parse_init();
#endif
	TileMapLayer::navmesh_parse_init();
#ifndef PHYSICS_2D_DISABLED
	StaticBody2D::navmesh_parse_init();
#endif // PHYSICS_2D_DISABLED
#endif // NAVIGATION_2D_DISABLED

#ifndef NAVIGATION_3D_DISABLED
	// 3D nodes that support navmesh baking need to server register their source geometry parsers.
	MeshInstance3D::navmesh_parse_init();
	MultiMeshInstance3D::navmesh_parse_init();
	NavigationObstacle3D::navmesh_parse_init();
#ifndef PHYSICS_3D_DISABLED
	StaticBody3D::navmesh_parse_init();
#endif // PHYSICS_3D_DISABLED
#endif // NAVIGATION_3D_DISABLED

#if !defined(NAVIGATION_2D_DISABLED) || !defined(NAVIGATION_3D_DISABLED)
	OS::get_singleton()->yield(); // may take time to init
#endif // !defined(NAVIGATION_2D_DISABLED) || !defined(NAVIGATION_3D_DISABLED)

	OS::get_singleton()->yield(); // may take time to init

	for (int i = 0; i < 20; i++) {
		GLOBAL_DEF_BASIC(vformat("%s/layer_%d", PNAME("layer_names/2d_render"), i + 1), "");
		GLOBAL_DEF_BASIC(vformat("%s/layer_%d", PNAME("layer_names/3d_render"), i + 1), "");
	}

	for (int i = 0; i < 32; i++) {
		GLOBAL_DEF_BASIC(vformat("%s/layer_%d", PNAME("layer_names/2d_physics"), i + 1), "");
#ifndef NAVIGATION_2D_DISABLED
		GLOBAL_DEF_BASIC(vformat("%s/layer_%d", PNAME("layer_names/2d_navigation"), i + 1), "");
#endif // NAVIGATION_2D_DISABLED
		GLOBAL_DEF_BASIC(vformat("%s/layer_%d", PNAME("layer_names/3d_physics"), i + 1), "");
#ifndef NAVIGATION_3D_DISABLED
		GLOBAL_DEF_BASIC(vformat("%s/layer_%d", PNAME("layer_names/3d_navigation"), i + 1), "");
#endif // NAVIGATION_3D_DISABLED
	}

	for (int i = 0; i < 32; i++) {
		GLOBAL_DEF_BASIC(vformat("%s/layer_%d", PNAME("layer_names/avoidance"), i + 1), "");
	}

	if (RenderingServer::get_singleton()) {
		// RenderingServer needs to exist for this to succeed.
		ColorPickerShape::init_shaders();
		GraphEdit::init_shaders();
	}

	SceneDebugger::initialize();

	OS::get_singleton()->benchmark_end_measure("Scene", "Register Types");
}

void unregister_scene_types()
{
	OS::get_singleton()->benchmark_begin_measure("Scene", "Unregister Types");

	SceneDebugger::deinitialize();

	ResourceLoader::remove_resource_format_loader(resource_loader_texture_layered);
	resource_loader_texture_layered.unref();
	ResourceLoader::remove_resource_format_loader(resource_loader_texture_3d);
	resource_loader_texture_3d.unref();

	ResourceLoader::remove_resource_format_loader(resource_loader_stream_texture);
	resource_loader_stream_texture.unref();

	ResourceSaver::remove_resource_format_saver(resource_saver_text);
	resource_saver_text.unref();

	ResourceLoader::remove_resource_format_loader(resource_loader_text);
	resource_loader_text.unref();

	ResourceSaver::remove_resource_format_saver(resource_saver_shader);
	resource_saver_shader.unref();

	ResourceLoader::remove_resource_format_loader(resource_loader_shader);
	resource_loader_shader.unref();
	ResourceSaver::remove_resource_format_saver(resource_saver_shader_include);
	resource_saver_shader_include.unref();

	ResourceLoader::remove_resource_format_loader(resource_loader_shader_include);
	resource_loader_shader_include.unref();

	// StandardMaterial3D is not initialized when 3D is disabled, so it shouldn't be cleaned up
	// either
#ifndef _3D_DISABLED
	BaseMaterial3D::finish_shaders();
	PhysicalSkyMaterial::cleanup_shader();
	PanoramaSkyMaterial::cleanup_shader();
	ProceduralSkyMaterial::cleanup_shader();
	FogMaterial::cleanup_shader();
#endif // _3D_DISABLED

	ParticleProcessMaterial::finish_shaders();
	CanvasItemMaterial::finish_shaders();
	ColorPickerShape::finish_shaders();
	BlitMaterial::cleanup_shader();
	GraphEdit::finish_shaders();
	SceneStringNames::free();

	OS::get_singleton()->benchmark_end_measure("Scene", "Unregister Types");
}

void register_scene_singletons()
{
	OS::get_singleton()->benchmark_begin_measure("Scene", "Register Singletons");

	Engine::get_singleton()->add_singleton(
		Engine::Singleton("ThemeDB", ThemeDB::get_singleton()->obj.get()));

	OS::get_singleton()->benchmark_end_measure("Scene", "Register Singletons");
}


