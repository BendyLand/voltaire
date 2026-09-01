/**************************************************************************/
/*  resource_importer_scene.h                                             */
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

#pragma once

#include "core/error/error_macros.h"
#include "core/io/resource_importer.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/resources/3d/box_shape_3d.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/3d/cylinder_shape_3d.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/3d/sphere_shape_3d.h"
#include "scene/resources/animation.h"
#include "scene/resources/mesh.h"

class AnimationPlayer;
class ImporterMesh;
class Material;

class EditorSceneFormatImporter : public RefCounted
{
	List<ResourceImporter::ImportOption>* current_option_list = nullptr;

protected:
	static void _bind_methods();

public:
	enum ImportFlags
	{
		IMPORT_SCENE = 1,
		IMPORT_ANIMATION = 2,
		IMPORT_FAIL_ON_MISSING_DEPENDENCIES = 4,
		IMPORT_GENERATE_TANGENT_ARRAYS = 8,
		IMPORT_USE_NAMED_SKIN_BINDS = 16,
		IMPORT_DISCARD_MESHES_AND_MATERIALS = 32, // used for optimizing animation import
		IMPORT_FORCE_DISABLE_MESH_COMPRESSION = 64,
	};

	virtual void get_extensions(List<String>* r_extensions) const;
	virtual void get_import_options(
		const String& p_path, List<ResourceImporter::ImportOption>* r_options);
};

class EditorScenePostImport : public RefCounted
{
	String source_file;

protected:
	static void _bind_methods();

public:
	String get_source_file() const;
	virtual Node* post_import(Node* p_scene);
	virtual void init(const String& p_source_file);
};

class EditorScenePostImportPlugin : public RefCounted
{
public:
	enum InternalImportCategory
	{
		INTERNAL_IMPORT_CATEGORY_NODE,
		INTERNAL_IMPORT_CATEGORY_MESH_3D_NODE,
		INTERNAL_IMPORT_CATEGORY_MESH,
		INTERNAL_IMPORT_CATEGORY_MATERIAL,
		INTERNAL_IMPORT_CATEGORY_ANIMATION,
		INTERNAL_IMPORT_CATEGORY_ANIMATION_NODE,
		INTERNAL_IMPORT_CATEGORY_SKELETON_3D_NODE,
		INTERNAL_IMPORT_CATEGORY_MAX
	};

private:
	List<ResourceImporter::ImportOption>* current_option_list = nullptr;

protected:
	static void _bind_methods();

public:
	virtual void get_internal_import_options(
		InternalImportCategory p_category, List<ResourceImporter::ImportOption>* r_options);

	virtual void get_import_options(
		const String& p_path, List<ResourceImporter::ImportOption>* r_options);
};

class ResourceImporterScene : public ResourceImporter
{
	static Vector<Ref<EditorSceneFormatImporter>> scene_importers;
	static Vector<Ref<EditorScenePostImportPlugin>> post_importer_plugins;

	enum LightBakeMode
	{
		LIGHT_BAKE_DISABLED,
		LIGHT_BAKE_STATIC,
		LIGHT_BAKE_STATIC_LIGHTMAPS,
		LIGHT_BAKE_DYNAMIC,
	};

	enum MeshPhysicsMode
	{
		MESH_PHYSICS_DISABLED,
		MESH_PHYSICS_MESH_AND_STATIC_COLLIDER,
		MESH_PHYSICS_RIGID_BODY_AND_MESH,
		MESH_PHYSICS_STATIC_COLLIDER_ONLY,
		MESH_PHYSICS_AREA_ONLY,
	};

	enum NavMeshMode
	{
		NAVMESH_DISABLED,
		NAVMESH_MESH_AND_NAVMESH,
		NAVMESH_NAVMESH_ONLY,
	};

	enum OccluderMode
	{
		OCCLUDER_DISABLED,
		OCCLUDER_MESH_AND_OCCLUDER,
		OCCLUDER_OCCLUDER_ONLY,
	};

	enum MeshOverride
	{
		MESH_OVERRIDE_DEFAULT,
		MESH_OVERRIDE_ENABLE,
		MESH_OVERRIDE_DISABLE,
	};

	enum BodyType
	{
		BODY_TYPE_STATIC,
		BODY_TYPE_DYNAMIC,
		BODY_TYPE_AREA
	};

	enum ShapeType
	{
		SHAPE_TYPE_DECOMPOSE_CONVEX,
		SHAPE_TYPE_SIMPLE_CONVEX,
		SHAPE_TYPE_TRIMESH,
		SHAPE_TYPE_BOX,
		SHAPE_TYPE_SPHERE,
		SHAPE_TYPE_CYLINDER,
		SHAPE_TYPE_CAPSULE,
		SHAPE_TYPE_AUTOMATIC,
	};

	void _replace_owner(Node* p_node, Node* p_scene, Node* p_new_owner);
	void _add_shapes(Node* p_node, const Vector<Ref<Shape3D>>& p_shapes);

	enum AnimationImportTracks
	{
		ANIMATION_IMPORT_TRACKS_IF_PRESENT,
		ANIMATION_IMPORT_TRACKS_IF_PRESENT_FOR_ALL,
		ANIMATION_IMPORT_TRACKS_NEVER,
	};

	enum TrackChannel
	{
		TRACK_CHANNEL_POSITION,
		TRACK_CHANNEL_ROTATION,
		TRACK_CHANNEL_SCALE,
		TRACK_CHANNEL_BLEND_SHAPE,
		TRACK_CHANNEL_MAX
	};

	void _optimize_track_usage(AnimationPlayer* p_player, AnimationImportTracks* p_track_actions);

	String _scene_import_type = "PackedScene";

public:
	static const String material_extension[3];

	static void add_post_importer_plugin(
		const Ref<EditorScenePostImportPlugin>& p_plugin, bool p_first_priority = false);
	static void remove_post_importer_plugin(const Ref<EditorScenePostImportPlugin>& p_plugin);

	const Vector<Ref<EditorSceneFormatImporter>>& get_scene_importers() const
	{
		return scene_importers;
	}

	static void add_scene_importer(
		Ref<EditorSceneFormatImporter> p_importer, bool p_first_priority = false);
	static void remove_scene_importer(Ref<EditorSceneFormatImporter> p_importer);
	static void get_scene_importer_extensions(List<String>* p_extensions);

	static void clean_up_importer_plugins();

	String get_scene_import_type() const { return _scene_import_type; }

	void set_scene_import_type(const String& p_type) { _scene_import_type = p_type; }

	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;
	virtual int get_format_version() const override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	enum InternalImportCategory
	{
		INTERNAL_IMPORT_CATEGORY_NODE = EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_NODE,
		INTERNAL_IMPORT_CATEGORY_MESH_3D_NODE =
			EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_MESH_3D_NODE,
		INTERNAL_IMPORT_CATEGORY_MESH = EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_MESH,
		INTERNAL_IMPORT_CATEGORY_MATERIAL =
			EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_MATERIAL,
		INTERNAL_IMPORT_CATEGORY_ANIMATION =
			EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_ANIMATION,
		INTERNAL_IMPORT_CATEGORY_ANIMATION_NODE =
			EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_ANIMATION_NODE,
		INTERNAL_IMPORT_CATEGORY_SKELETON_3D_NODE =
			EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_SKELETON_3D_NODE,
		INTERNAL_IMPORT_CATEGORY_MAX = EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_MAX
	};

	void get_internal_import_options(
		InternalImportCategory p_category, List<ImportOption>* r_options) const;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;

	// Import scenes *after* everything else (such as textures).
	virtual int get_import_order() const override { return ResourceImporter::IMPORT_ORDER_SCENE; }

	Ref<Animation> _save_animation_to_file(Ref<Animation> anim, bool p_save_to_file,
		const String& p_save_to_path, bool p_keep_custom_tracks);
	void _optimize_animations(
		AnimationPlayer* anim, float p_max_vel_error, float p_max_ang_error, int p_prc_error);
	void _compress_animations(AnimationPlayer* anim, int p_page_size_kb);

	virtual bool has_advanced_options() const override;
	virtual void show_advanced_options(const String& p_path) override;

	ResourceImporterScene(const String& p_scene_import_type = "PackedScene");

	template <typename M>
	static Vector<Ref<Shape3D>> get_collision_shapes(
		const Ref<ImporterMesh>& p_mesh, const M& p_options, float p_applied_root_scale);

	template <typename M> static Transform3D get_collision_shapes_transform(const M& p_options);
};

class EditorSceneFormatImporterESCN : public EditorSceneFormatImporter
{
public:
	virtual void get_extensions(List<String>* r_extensions) const override;
};

template <typename M>
Vector<Ref<Shape3D>> ResourceImporterScene::get_collision_shapes(
	const Ref<ImporterMesh>& p_mesh, const M& p_options, float p_applied_root_scale)
{
	ERR_FAIL_COND_V(p_mesh.is_null(), Vector<Ref<Shape3D>>());

	ShapeType generate_shape_type = SHAPE_TYPE_AUTOMATIC;
	if (p_options.has(SNAME("physics/shape_type"))) {
		generate_shape_type = (ShapeType)p_options[SNAME("physics/shape_type")].operator int();
	}

	if (generate_shape_type == SHAPE_TYPE_AUTOMATIC) {
		BodyType body_type = BODY_TYPE_STATIC;
		if (p_options.has(SNAME("physics/body_type"))) {
			body_type = (BodyType)p_options[SNAME("physics/body_type")].operator int();
		}

		generate_shape_type =
			body_type == BODY_TYPE_DYNAMIC ? SHAPE_TYPE_DECOMPOSE_CONVEX : SHAPE_TYPE_TRIMESH;
	}

	if (generate_shape_type == SHAPE_TYPE_DECOMPOSE_CONVEX) {
		Ref<MeshConvexDecompositionSettings> decomposition_settings =
			Ref<MeshConvexDecompositionSettings>();
		decomposition_settings.instantiate();
		bool advanced = false;
		if (p_options.has(SNAME("decomposition/advanced"))) {
			advanced = p_options[SNAME("decomposition/advanced")];
		}

		if (advanced) {
			if (p_options.has(SNAME("decomposition/max_concavity"))) {
				decomposition_settings->set_max_concavity(
					p_options[SNAME("decomposition/max_concavity")]);
			}

			if (p_options.has(SNAME("decomposition/symmetry_planes_clipping_bias"))) {
				decomposition_settings->set_symmetry_planes_clipping_bias(
					p_options[SNAME("decomposition/symmetry_planes_clipping_bias")]);
			}

			if (p_options.has(SNAME("decomposition/revolution_axes_clipping_bias"))) {
				decomposition_settings->set_revolution_axes_clipping_bias(
					p_options[SNAME("decomposition/revolution_axes_clipping_bias")]);
			}

			if (p_options.has(SNAME("decomposition/min_volume_per_convex_hull"))) {
				decomposition_settings->set_min_volume_per_convex_hull(
					p_options[SNAME("decomposition/min_volume_per_convex_hull")]);
			}

			if (p_options.has(SNAME("decomposition/resolution"))) {
				decomposition_settings->set_resolution(
					p_options[SNAME("decomposition/resolution")]);
			}

			if (p_options.has(SNAME("decomposition/max_num_vertices_per_convex_hull"))) {
				decomposition_settings->set_max_num_vertices_per_convex_hull(
					p_options[SNAME("decomposition/max_num_vertices_per_convex_hull")]);
			}

			if (p_options.has(SNAME("decomposition/plane_downsampling"))) {
				decomposition_settings->set_plane_downsampling(
					p_options[SNAME("decomposition/plane_downsampling")]);
			}

			if (p_options.has(SNAME("decomposition/convexhull_downsampling"))) {
				decomposition_settings->set_convex_hull_downsampling(
					p_options[SNAME("decomposition/convexhull_downsampling")]);
			}

			if (p_options.has(SNAME("decomposition/normalize_mesh"))) {
				decomposition_settings->set_normalize_mesh(
					p_options[SNAME("decomposition/normalize_mesh")]);
			}

			if (p_options.has(SNAME("decomposition/mode"))) {
				decomposition_settings->set_mode(
					(MeshConvexDecompositionSettings::Mode)p_options[SNAME("decomposition/mode")].
					operator int());
			}

			if (p_options.has(SNAME("decomposition/convexhull_approximation"))) {
				decomposition_settings->set_convex_hull_approximation(
					p_options[SNAME("decomposition/convexhull_approximation")]);
			}

			if (p_options.has(SNAME("decomposition/max_convex_hulls"))) {
				decomposition_settings->set_max_convex_hulls(
					MAX(1, (int)p_options[SNAME("decomposition/max_convex_hulls")]));
			}

			if (p_options.has(SNAME("decomposition/project_hull_vertices"))) {
				decomposition_settings->set_project_hull_vertices(
					p_options[SNAME("decomposition/project_hull_vertices")]);
			}
		}
		else {
			int precision_level = 5;
			if (p_options.has(SNAME("decomposition/precision"))) {
				precision_level = p_options[SNAME("decomposition/precision")];
			}

			const real_t precision = real_t(precision_level - 1) / 9.0;

			decomposition_settings->set_max_concavity(
				Math::lerp(real_t(1.0), real_t(0.001), precision));
			decomposition_settings->set_min_volume_per_convex_hull(
				Math::lerp(real_t(0.01), real_t(0.0001), precision));
			decomposition_settings->set_resolution(Math::lerp(10'000, 100'000, precision));
			decomposition_settings->set_max_num_vertices_per_convex_hull(
				Math::lerp(32, 64, precision));
			decomposition_settings->set_plane_downsampling(Math::lerp(3, 16, precision));
			decomposition_settings->set_convex_hull_downsampling(Math::lerp(3, 16, precision));
			decomposition_settings->set_max_convex_hulls(Math::lerp(1, 32, precision));
		}

		return p_mesh->convex_decompose(decomposition_settings);
	}
	else if (generate_shape_type == SHAPE_TYPE_SIMPLE_CONVEX) {
		Vector<Ref<Shape3D>> shapes;
		shapes.push_back(p_mesh->create_convex_shape(true,
			/*Passing false, otherwise VHACD will be used to simplify (Decompose) the Mesh.*/
			false));
		return shapes;
	}
	else if (generate_shape_type == SHAPE_TYPE_TRIMESH) {
		Vector<Ref<Shape3D>> shapes;
		shapes.push_back(p_mesh->create_trimesh_shape());
		return shapes;
	}
	else if (generate_shape_type == SHAPE_TYPE_BOX) {
		Ref<BoxShape3D> box;
		box.instantiate();
		if (p_options.has(SNAME("primitive/size"))) {
			box->set_size(
				p_options[SNAME("primitive/size")].operator Vector3() * p_applied_root_scale);
		}
		else {
			box->set_size(Vector3(2, 2, 2) * p_applied_root_scale);
		}

		Vector<Ref<Shape3D>> shapes;
		shapes.push_back(box);
		return shapes;

	}
	else if (generate_shape_type == SHAPE_TYPE_SPHERE) {
		Ref<SphereShape3D> sphere;
		sphere.instantiate();
		if (p_options.has(SNAME("primitive/radius"))) {
			sphere->set_radius(
				p_options[SNAME("primitive/radius")].operator float() * p_applied_root_scale);
		}
		else {
			sphere->set_radius(1.0f * p_applied_root_scale);
		}

		Vector<Ref<Shape3D>> shapes;
		shapes.push_back(sphere);
		return shapes;
	}
	else if (generate_shape_type == SHAPE_TYPE_CYLINDER) {
		Ref<CylinderShape3D> cylinder;
		cylinder.instantiate();
		if (p_options.has(SNAME("primitive/height"))) {
			cylinder->set_height(
				p_options[SNAME("primitive/height")].operator float() * p_applied_root_scale);
		}
		else {
			cylinder->set_height(1.0f * p_applied_root_scale);
		}
		if (p_options.has(SNAME("primitive/radius"))) {
			cylinder->set_radius(
				p_options[SNAME("primitive/radius")].operator float() * p_applied_root_scale);
		}
		else {
			cylinder->set_radius(1.0f * p_applied_root_scale);
		}

		Vector<Ref<Shape3D>> shapes;
		shapes.push_back(cylinder);
		return shapes;
	}
	else if (generate_shape_type == SHAPE_TYPE_CAPSULE) {
		Ref<CapsuleShape3D> capsule;
		capsule.instantiate();
		if (p_options.has(SNAME("primitive/height"))) {
			capsule->set_height(
				p_options[SNAME("primitive/height")].operator float() * p_applied_root_scale);
		}
		else {
			capsule->set_height(1.0f * p_applied_root_scale);
		}
		if (p_options.has(SNAME("primitive/radius"))) {
			capsule->set_radius(
				p_options[SNAME("primitive/radius")].operator float() * p_applied_root_scale);
		}
		else {
			capsule->set_radius(1.0f * p_applied_root_scale);
		}

		Vector<Ref<Shape3D>> shapes;
		shapes.push_back(capsule);
		return shapes;
	}
	return Vector<Ref<Shape3D>>();
}

template <typename M>
Transform3D ResourceImporterScene::get_collision_shapes_transform(const M& p_options)
{
	Transform3D transform;

	ShapeType generate_shape_type = SHAPE_TYPE_AUTOMATIC;
	if (p_options.has(SNAME("physics/shape_type"))) {
		generate_shape_type = (ShapeType)p_options[SNAME("physics/shape_type")].operator int();
	}

	if (generate_shape_type == SHAPE_TYPE_AUTOMATIC) {
		BodyType body_type = BODY_TYPE_STATIC;
		if (p_options.has(SNAME("physics/body_type"))) {
			body_type = (BodyType)p_options[SNAME("physics/body_type")].operator int();
		}

		generate_shape_type =
			body_type == BODY_TYPE_DYNAMIC ? SHAPE_TYPE_DECOMPOSE_CONVEX : SHAPE_TYPE_TRIMESH;
	}

	if (generate_shape_type == SHAPE_TYPE_BOX || generate_shape_type == SHAPE_TYPE_SPHERE ||
		generate_shape_type == SHAPE_TYPE_CYLINDER || generate_shape_type == SHAPE_TYPE_CAPSULE) {
		if (p_options.has(SNAME("primitive/position"))) {
			transform.origin = p_options[SNAME("primitive/position")];
		}

		if (p_options.has(SNAME("primitive/rotation"))) {
			transform.basis = Basis::from_euler(
				p_options[SNAME("primitive/rotation")].operator Vector3() * (Math::PI / 180.0));
		}
	}
	return transform;
}


