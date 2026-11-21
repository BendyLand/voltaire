/**************************************************************************/
/*  resource_importer_scene.cpp                                           */
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

#include "core/error/error_macros.h"
#include "core/io/dir_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/settings/editor_settings.h"
#include "resource_importer_scene.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/navigation/navigation_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/3d/physics/area_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/collision_object_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/3d/mesh_library.h"
#include "scene/resources/3d/shape_3d.h"
#include "scene/resources/animation.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/physics_material.h"
#include "scene/resources/packed_scene.h"

void EditorSceneFormatImporter::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options)
{
	current_option_list = r_options;
	current_option_list = nullptr;
}

String EditorScenePostImport::get_source_file() const { return source_file; }

void EditorScenePostImport::init(const String& p_source_file) { source_file = p_source_file; }

void EditorScenePostImportPlugin::get_internal_import_options(
	InternalImportCategory p_category, List<ResourceImporter::ImportOption>* r_options)
{
	current_option_list = r_options;
	current_option_list = nullptr;
}

void EditorScenePostImportPlugin::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options)
{
	current_option_list = r_options;
	current_option_list = nullptr;
}

const String ResourceImporterScene::material_extension[3] = {".tres", ".res", ".material"};

String ResourceImporterScene::get_importer_name() const
{
	// For compatibility with 4.2 and earlier we need to keep the "scene" and "animation_library"
	// names. However this is arbitrary so for new import types we can use any string.
	if (_scene_import_type == "PackedScene") {
		return "scene";
	}
	else if (_scene_import_type == "AnimationLibrary") {
		return "animation_library";
	}
	return _scene_import_type;
}

String ResourceImporterScene::get_visible_name() const
{
	// This is displayed on the UI. Friendly names here are nice but not vital, so fall back to the
	// type.
	if (_scene_import_type == "PackedScene") {
		return "Scene";
	}
	else if (_scene_import_type == "ArrayMesh") {
		return "Single Mesh";
	}
	return _scene_import_type.capitalize();
}

void ResourceImporterScene::get_recognized_extensions(List<String>* p_extensions) const
{
	get_scene_importer_extensions(p_extensions);
}

String ResourceImporterScene::get_save_extension() const
{
	if (_scene_import_type == "PackedScene") {
		return "scn";
	}
	return "res";
}

String ResourceImporterScene::get_resource_type() const { return _scene_import_type; }

int ResourceImporterScene::get_format_version() const { return 1; }

int ResourceImporterScene::get_preset_count() const { return 0; }

String ResourceImporterScene::get_preset_name(int p_idx) const { return String(); }

static bool _teststr(const String& p_what, const String& p_str)
{
	String what = p_what;

	// Remove trailing spaces and numbers, some apps like blender add ".number" to duplicates
	// (dot is replaced with _ as invalid character) so also compensate for this.
	while (what.length() && (is_digit(what[what.length() - 1]) || what[what.length() - 1] <= 32 ||
								what[what.length() - 1] == '_')) {
		what = what.substr(0, what.length() - 1);
	}

	if (what.containsn("$" + p_str)) { // Blender and other stuff.
		return true;
	}
	if (what.to_lower().ends_with(
			"-" + p_str)) { // collada only supports "_" and "-" besides letters
		return true;
	}
	if (what.to_lower().ends_with(
			"_" + p_str)) { // collada only supports "_" and "-" besides letters
		return true;
	}
	return false;
}

static String _fixstr(const String& p_what, const String& p_str)
{
	String what = p_what;

	// Remove trailing spaces and numbers, some apps like blender add ".number" to duplicates
	// (dot is replaced with _ as invalid character) so also compensate for this.
	while (what.length() && (is_digit(what[what.length() - 1]) || what[what.length() - 1] <= 32 ||
								what[what.length() - 1] == '_')) {
		what = what.substr(0, what.length() - 1);
	}

	String end = p_what.substr(what.length());

	if (what.containsn("$" + p_str)) { // Blender and other stuff.
		return what.replace("$" + p_str, "") + end;
	}
	if (what.to_lower().ends_with(
			"-" + p_str)) { // collada only supports "_" and "-" besides letters
		return what.substr(0, what.length() - (p_str.length() + 1)) + end;
	}
	if (what.to_lower().ends_with(
			"_" + p_str)) { // collada only supports "_" and "-" besides letters
		return what.substr(0, what.length() - (p_str.length() + 1)) + end;
	}
	return what;
}

static void _pre_gen_shape_list(
	Ref<ImporterMesh>& mesh, Vector<Ref<Shape3D>>& r_shape_list, bool p_convex)
{
	ERR_FAIL_COND_MSG(mesh.is_null(), "Cannot generate shape list with null mesh value.");
	if (!p_convex) {
		Ref<Shape3D> shape = mesh->create_trimesh_shape();
		r_shape_list.push_back(shape);
	}
	else {
		Vector<Ref<Shape3D>> cd;
		cd.push_back(mesh->create_convex_shape(true,
			/*Passing false, otherwise VHACD will be used to simplify (Decompose) the Mesh.*/
			false));
		if (cd.size()) {
			for (int i = 0; i < cd.size(); i++) {
				r_shape_list.push_back(cd[i]);
			}
		}
	}
}

struct ScalableNodeCollection
{
	HashSet<Node3D*> node_3ds;
	HashSet<Ref<ImporterMesh>> importer_meshes;
	HashSet<Ref<Skin>> skins;
	HashSet<Ref<Animation>> animations;
};

void _rescale_skin(Vector3 p_scale, Ref<Skin> p_skin)
{
	// MESH and SKIN data divide, to compensate for object position multiplying.
	for (int i = 0; i < p_skin->get_bind_count(); i++) {
		Transform3D transform = p_skin->get_bind_pose(i);
		p_skin->set_bind_pose(i, Transform3D(transform.basis, p_scale * transform.origin));
	}
}

void ResourceImporterScene::_optimize_animations(
	AnimationPlayer* anim, float p_max_vel_error, float p_max_ang_error, int p_prc_error)
{
	LocalVector<String> anim_names;
	anim->get_animation_list(&anim_names);
	for (const String& E : anim_names) {
		Ref<Animation> a = anim->get_animation(E);
		a->optimize(p_max_vel_error, p_max_ang_error, p_prc_error);
	}
}

void ResourceImporterScene::_compress_animations(AnimationPlayer* anim, int p_page_size_kb)
{
	LocalVector<String> anim_names;
	anim->get_animation_list(&anim_names);
	for (const String& E : anim_names) {
		Ref<Animation> a = anim->get_animation(E);
		a->compress(p_page_size_kb * 1024);
	}
}

void ResourceImporterScene::_replace_owner(Node* p_node, Node* p_scene, Node* p_new_owner)
{
	if (p_node != p_new_owner && p_node->get_owner() == p_scene) {
		p_node->set_owner(p_new_owner);
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node* n = p_node->get_child(i);
		_replace_owner(n, p_scene, p_new_owner);
	}
}

void ResourceImporterScene::_add_shapes(Node* p_node, const Vector<Ref<Shape3D>>& p_shapes)
{
	for (const Ref<Shape3D>& E : p_shapes) {
		CollisionShape3D* cshape = memnew(CollisionShape3D);
		cshape->set_shape(E);
		p_node->add_child(cshape, true);

		cshape->set_owner(p_node->get_owner());
	}
}

Vector<Ref<EditorSceneFormatImporter>> ResourceImporterScene::scene_importers;
Vector<Ref<EditorScenePostImportPlugin>> ResourceImporterScene::post_importer_plugins;

bool ResourceImporterScene::has_advanced_options() const { return false; }

void ResourceImporterScene::show_advanced_options(const String& p_path) {}

ResourceImporterScene::ResourceImporterScene(const String& p_scene_import_type)
{
	_scene_import_type = p_scene_import_type;
}

void ResourceImporterScene::add_scene_importer(
	Ref<EditorSceneFormatImporter> p_importer, bool p_first_priority)
{
	ERR_FAIL_COND(p_importer.is_null());
	if (p_first_priority) {
		scene_importers.insert(0, p_importer);
	}
	else {
		scene_importers.push_back(p_importer);
	}
}

void ResourceImporterScene::remove_post_importer_plugin(
	const Ref<EditorScenePostImportPlugin>& p_plugin)
{
	post_importer_plugins.erase(p_plugin);
}

void ResourceImporterScene::add_post_importer_plugin(
	const Ref<EditorScenePostImportPlugin>& p_plugin, bool p_first_priority)
{
	ERR_FAIL_COND(p_plugin.is_null());
	if (p_first_priority) {
		post_importer_plugins.insert(0, p_plugin);
	}
	else {
		post_importer_plugins.push_back(p_plugin);
	}
}

void ResourceImporterScene::remove_scene_importer(Ref<EditorSceneFormatImporter> p_importer)
{
	scene_importers.erase(p_importer);
}

void ResourceImporterScene::clean_up_importer_plugins()
{
	scene_importers.clear();
	post_importer_plugins.clear();
}

void ResourceImporterScene::get_scene_importer_extensions(List<String>* p_extensions)
{
	for (Ref<EditorSceneFormatImporter> importer_elem : scene_importers) {
		importer_elem->get_extensions(p_extensions);
	}
}

void EditorSceneFormatImporterESCN::get_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("escn");
}

Node* EditorScenePostImport::post_import(Node* p_scene) { return p_scene; }

void ResourceImporterScene::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}

void EditorSceneFormatImporter::get_extensions(List<String>* r_extensions) const {}



// ============================================================================
// Merged from editor/import/3d/collada.cpp
// ============================================================================
// --- Inlined from editor/import/3d/collada.h ---
#include "core/io/xml_parser.h"

class Collada
{
public:
	enum ImportFlags
	{
		IMPORT_FLAG_SCENE = 1,
		IMPORT_FLAG_ANIMATION = 2
	};

	struct Image
	{
		String path;
	};

	struct Material
	{
		String name;
		String instance_effect;
	};

	struct Effect
	{
		String name;

		struct Channel
		{
			int uv_idx = 0;
			String texture;
			Color color;
		};

		Channel diffuse, specular, emission, bump;
		float shininess = 40;
		bool found_double_sided = false;
		bool double_sided = true;
		bool unshaded = false;

		String get_texture_path(const String& p_source, Collada& p_state) const;

		Effect() { diffuse.color = Color(1, 1, 1, 1); }
	};

	struct CameraData
	{
		enum Mode
		{
			MODE_PERSPECTIVE,
			MODE_ORTHOGONAL
		};

		Mode mode = MODE_PERSPECTIVE;

		union
		{
			struct
			{
				float x_fov = 0;
				float y_fov = 0;
			} perspective;

			struct
			{
				float x_mag = 0;
				float y_mag = 0;
			} orthogonal;
		};

		float aspect = 1;
		float z_near = 0.05;
		float z_far = 4000;

		CameraData() {}
	};

	struct LightData
	{
		enum Mode
		{
			MODE_AMBIENT,
			MODE_DIRECTIONAL,
			MODE_OMNI,
			MODE_SPOT
		};

		Mode mode = MODE_AMBIENT;

		Color color = Color(1, 1, 1, 1);

		float constant_att = 0;
		float linear_att = 0;
		float quad_att = 0;

		float spot_angle = 45;
		float spot_exp = 1;
	};

	struct MeshData
	{
		String name;

		struct Source
		{
			Vector<float> array;
			int stride = 0;
		};

		HashMap<String, Source> sources;

		struct Vertices
		{
			HashMap<String, String> sources;
		};

		HashMap<String, Vertices> vertices;

		struct Primitives
		{
			struct SourceRef
			{
				String source;
				int offset = 0;
			};

			String material;
			HashMap<String, SourceRef> sources;
			Vector<float> polygons;
			Vector<float> indices;
			int count = 0;
			int vertex_size = 0;
		};

		Vector<Primitives> primitives;

		bool found_double_sided = false;
		bool double_sided = true;
	};

	struct CurveData
	{
		String name;
		bool closed = false;

		struct Source
		{
			Vector<String> sarray;
			Vector<float> array;
			int stride = 0;
		};

		HashMap<String, Source> sources;

		HashMap<String, String> control_vertices;
	};

	struct SkinControllerData
	{
		String base;
		bool use_idrefs = false;

		Transform3D bind_shape;

		struct Source
		{
			Vector<String> sarray; // maybe for names
			Vector<float> array;
			int stride = 1;
		};

		HashMap<String, Source> sources;

		struct Joints
		{
			HashMap<String, String> sources;
		} joints;

		struct Weights
		{
			struct SourceRef
			{
				String source;
				int offset = 0;
			};

			String material;
			HashMap<String, SourceRef> sources;
			Vector<float> sets;
			Vector<float> indices;
			int count = 0;
		} weights;

		HashMap<String, Transform3D> bone_rest_map;
	};

	struct MorphControllerData
	{
		String mesh;
		String mode;

		struct Source
		{
			int stride = 1;
			Vector<String> sarray; // maybe for names
			Vector<float> array;
		};

		HashMap<String, Source> sources;

		HashMap<String, String> targets;
	};

	struct Vertex
	{
		int idx = 0;
		Vector3 vertex;
		Vector3 normal;
		Vector3 uv;
		Vector3 uv2;
		Plane tangent;
		Color color;
		int uid = 0;

		struct Weight
		{
			int bone_idx = 0;
			float weight = 0;

			bool operator<(const Weight w) const { return weight > w.weight; } // heaviest first
		};

		Vector<Weight> weights;

		void fix_weights()
		{
			weights.sort();
			if (weights.size() > 4) {
				// cap to 4 and make weights add up 1
				weights.resize(4);
				float total = 0;
				for (int i = 0; i < 4; i++) {
					total += weights[i].weight;
				}
				if (total) {
					for (int i = 0; i < 4; i++) {
						weights.write[i].weight /= total;
					}
				}
			}
		}

		void fix_unit_scale(const Collada& p_state);

		bool operator<(const Vertex& p_vert) const
		{
			if (uid == p_vert.uid) {
				if (vertex == p_vert.vertex) {
					if (normal == p_vert.normal) {
						if (uv == p_vert.uv) {
							if (uv2 == p_vert.uv2) {
								if (!weights.is_empty() || !p_vert.weights.is_empty()) {
									if (weights.size() == p_vert.weights.size()) {
										for (int i = 0; i < weights.size(); i++) {
											if (weights[i].bone_idx != p_vert.weights[i].bone_idx) {
												return weights[i].bone_idx <
													   p_vert.weights[i].bone_idx;
											}

											if (weights[i].weight != p_vert.weights[i].weight) {
												return weights[i].weight < p_vert.weights[i].weight;
											}
										}
									}
									else {
										return weights.size() < p_vert.weights.size();
									}
								}

								return (color < p_vert.color);
							}
							else {
								return (uv2 < p_vert.uv2);
							}
						}
						else {
							return (uv < p_vert.uv);
						}
					}
					else {
						return (normal < p_vert.normal);
					}
				}
				else {
					return vertex < p_vert.vertex;
				}
			}
			else {
				return uid < p_vert.uid;
			}
		}
	};

	struct Node
	{
		enum Type
		{
			TYPE_NODE,
			TYPE_JOINT,
			TYPE_SKELETON, // this bone is not collada, it's added afterwards as optimization
			TYPE_LIGHT,
			TYPE_CAMERA,
			TYPE_GEOMETRY
		};

		struct XForm
		{
			enum Op
			{
				OP_ROTATE,
				OP_SCALE,
				OP_TRANSLATE,
				OP_MATRIX,
				OP_VISIBILITY
			};

			String id;
			Op op = OP_ROTATE;
			Vector<float> data;
		};

		Type type = TYPE_NODE;

		String name;
		String id;
		String empty_draw_type;
		bool noname = false;
		Vector<XForm> xform_list;
		Transform3D default_transform;
		Transform3D post_transform;
		Vector<Node*> children;

		Node* parent = nullptr;

		Transform3D compute_transform(const Collada& p_state) const;
		Transform3D get_global_transform() const;
		Transform3D get_transform() const;

		bool ignore_anim = false;

		virtual ~Node()
		{
			for (int i = 0; i < children.size(); i++) {
				memdelete(children[i]);
			}
		}
	};

	struct NodeSkeleton : public Node
	{
		NodeSkeleton() { type = TYPE_SKELETON; }
	};

	struct NodeJoint : public Node
	{
		NodeSkeleton* owner = nullptr;
		String sid;

		NodeJoint() { type = TYPE_JOINT; }
	};

	struct NodeGeometry : public Node
	{
		bool controller = false;
		String source;

		struct Material
		{
			String target;
		};

		HashMap<String, Material> material_map;
		Vector<String> skeletons;

		NodeGeometry() { type = TYPE_GEOMETRY; }
	};

	struct NodeCamera : public Node
	{
		String camera;

		NodeCamera() { type = TYPE_CAMERA; }
	};

	struct NodeLight : public Node
	{
		String light;

		NodeLight() { type = TYPE_LIGHT; }
	};

	struct VisualScene
	{
		String name;
		Vector<Node*> root_nodes;

		~VisualScene()
		{
			for (int i = 0; i < root_nodes.size(); i++) {
				memdelete(root_nodes[i]);
			}
		}
	};

	struct AnimationClip
	{
		String name;
		float begin = 0;
		float end = 1;
		Vector<String> tracks;
	};

	struct AnimationTrack
	{
		String id;
		String target;
		String param;
		String component;
		bool property = false;

		enum InterpolationType
		{
			INTERP_LINEAR,
			INTERP_BEZIER
		};

		struct Key
		{
			enum Type
			{
				TYPE_FLOAT,
				TYPE_MATRIX
			};

			float time = 0;
			Vector<float> data;
			Point2 in_tangent;
			Point2 out_tangent;
			InterpolationType interp_type = INTERP_LINEAR;
		};

		Vector<float> get_value_at_time(float p_time) const;

		Vector<Key> keys;
	};

	/****************/
	/* IMPORT STATE */
	/****************/

	struct State
	{
		int import_flags = 0;

		float unit_scale = 1.0;
		Vector3::Axis up_axis = Vector3::AXIS_Y;
		bool z_up = false;

		struct Version
		{
			int major = 0, minor = 0, rev = 0;

			bool operator<(const Version& p_ver) const
			{
				return (major == p_ver.major)
						   ? ((minor == p_ver.minor) ? (rev < p_ver.rev) : minor < p_ver.minor)
						   : major < p_ver.major;
			}

			Version(int p_major = 0, int p_minor = 0, int p_rev = 0)
			{
				major = p_major;
				minor = p_minor;
				rev = p_rev;
			}
		} version;

		HashMap<String, CameraData> camera_data_map;
		HashMap<String, MeshData> mesh_data_map;
		HashMap<String, LightData> light_data_map;
		HashMap<String, CurveData> curve_data_map;

		HashMap<String, String> mesh_name_map;
		HashMap<String, String> morph_name_map;
		HashMap<String, String> morph_ownership_map;
		HashMap<String, SkinControllerData> skin_controller_data_map;
		HashMap<String, MorphControllerData> morph_controller_data_map;

		HashMap<String, Image> image_map;
		HashMap<String, Material> material_map;
		HashMap<String, Effect> effect_map;

		HashMap<String, VisualScene> visual_scene_map;
		HashMap<String, Node*> scene_map;
		HashSet<String> idref_joints;
		HashMap<String, String> sid_to_node_map;
		// RBMap<String,NodeJoint*> bone_map;

		HashMap<String, Transform3D> bone_rest_map;

		String local_path;
		String root_visual_scene;
		String root_physics_scene;

		Vector<AnimationClip> animation_clips;
		Vector<AnimationTrack> animation_tracks;
		HashMap<String, Vector<int>> referenced_tracks;
		HashMap<String, Vector<int>> by_id_tracks;

		float animation_length = 0;
	} state;

	Error load(const String& p_path, int p_flags = 0);

	Transform3D fix_transform(const Transform3D& p_transform);

	Transform3D get_root_transform() const;

	int get_uv_channel(const String& p_name);

private: // private stuff
	HashMap<String, int> channel_map;

	void _parse_asset(XMLParser& p_parser);
	void _parse_image(XMLParser& p_parser);
	void _parse_material(XMLParser& p_parser);
	void _parse_effect_material(XMLParser& p_parser, Effect& p_effect, String& p_id);
	void _parse_effect(XMLParser& p_parser);
	void _parse_camera(XMLParser& p_parser);
	void _parse_light(XMLParser& p_parser);
	void _parse_animation_clip(XMLParser& p_parser);

	void _parse_mesh_geometry(XMLParser& p_parser, const String& p_id, const String& p_name);
	void _parse_curve_geometry(XMLParser& p_parser, const String& p_id, const String& p_name);

	void _parse_skin_controller(XMLParser& p_parser, const String& p_id);
	void _parse_morph_controller(XMLParser& p_parser, const String& p_id);
	void _parse_controller(XMLParser& p_parser);

	Node* _parse_visual_instance_geometry(XMLParser& p_parser);
	Node* _parse_visual_instance_camera(XMLParser& p_parser);
	Node* _parse_visual_instance_light(XMLParser& p_parser);

	Node* _parse_visual_node_instance_data(XMLParser& p_parser);
	Node* _parse_visual_scene_node(XMLParser& p_parser);
	void _parse_visual_scene(XMLParser& p_parser);

	void _parse_animation(XMLParser& p_parser);
	void _parse_scene(XMLParser& p_parser);
	void _parse_library(XMLParser& p_parser);

	Vector<float> _read_float_array(XMLParser& p_parser);
	Vector<String> _read_string_array(XMLParser& p_parser);
	Transform3D _read_transform(XMLParser& p_parser);
	String _read_empty_draw_type(XMLParser& p_parser);

	void _joint_set_owner(Collada::Node* p_node, NodeSkeleton* p_owner);
	void _create_skeletons(Collada::Node** p_node, NodeSkeleton* p_skeleton = nullptr);
	void _find_morph_nodes(VisualScene* p_vscene, Node* p_node);
	bool _remove_node(Node* p_parent, Node* p_node);
	void _remove_node(VisualScene* p_vscene, Node* p_node);
	void _merge_skeletons2(VisualScene* p_vscene);
	void _merge_skeletons(VisualScene* p_vscene, Node* p_node);
	bool _optimize_skeletons(VisualScene* p_vscene, Node* p_node);

	bool _move_geometry_to_skeletons(VisualScene* p_vscene, Node* p_node, List<Node*>* p_mgeom);

	void _optimize();
};



// --- End editor/import/3d/collada.h ---
#include "core/config/project_settings.h"

// #define DEBUG_DEFAULT_ANIMATION
// #define DEBUG_COLLADA
#ifdef DEBUG_COLLADA
#define COLLADA_PRINT(m_what) print_line(m_what)
#else
#define COLLADA_PRINT(m_what)
#endif

#define COLLADA_IMPORT_SCALE_SCENE

/* HELPERS */

String Collada::Effect::get_texture_path(const String& p_source, Collada& p_state) const
{
	const String& image = p_source;
	ERR_FAIL_COND_V(!p_state.state.image_map.has(image), "");
	return p_state.state.image_map[image].path;
}

Transform3D Collada::get_root_transform() const
{
	Transform3D unit_scale_transform;
#ifndef COLLADA_IMPORT_SCALE_SCENE
	unit_scale_transform.scale(Vector3(state.unit_scale, state.unit_scale, state.unit_scale));
#endif
	return unit_scale_transform;
}

void Collada::Vertex::fix_unit_scale(const Collada& p_state)
{
#ifdef COLLADA_IMPORT_SCALE_SCENE
	vertex *= p_state.state.unit_scale;
#endif
}

static String _uri_to_id(const String& p_uri)
{
	if (p_uri.begins_with("#")) {
		return p_uri.substr(1);
	}
	else {
		return p_uri;
	}
}

/** HELPER FUNCTIONS **/

Transform3D Collada::fix_transform(const Transform3D& p_transform)
{
	Transform3D tr = p_transform;

#ifndef NO_UP_AXIS_SWAP

	if (state.up_axis != Vector3::AXIS_Y) {
		for (int i = 0; i < 3; i++) {
			SWAP(tr.basis[1][i], tr.basis[state.up_axis][i]);
		}
		for (int i = 0; i < 3; i++) {
			SWAP(tr.basis[i][1], tr.basis[i][state.up_axis]);
		}

		SWAP(tr.origin[1], tr.origin[state.up_axis]);

		tr.basis[state.up_axis][0] = -tr.basis[state.up_axis][0];
		tr.basis[state.up_axis][1] = -tr.basis[state.up_axis][1];
		tr.basis[0][state.up_axis] = -tr.basis[0][state.up_axis];
		tr.basis[1][state.up_axis] = -tr.basis[1][state.up_axis];
		tr.origin[state.up_axis] = -tr.origin[state.up_axis];
	}
#endif

	// tr.scale(Vector3(state.unit_scale.unit_scale.unit_scale));
	return tr;
	// return state.matrix_fix * p_transform;
}

static Transform3D _read_transform_from_array(const Vector<float>& p_array, int p_ofs = 0)
{
	Transform3D tr;
	// i wonder why collada matrices are transposed, given that's opposed to opengl..
	tr.basis.rows[0][0] = p_array[0 + p_ofs];
	tr.basis.rows[0][1] = p_array[1 + p_ofs];
	tr.basis.rows[0][2] = p_array[2 + p_ofs];
	tr.basis.rows[1][0] = p_array[4 + p_ofs];
	tr.basis.rows[1][1] = p_array[5 + p_ofs];
	tr.basis.rows[1][2] = p_array[6 + p_ofs];
	tr.basis.rows[2][0] = p_array[8 + p_ofs];
	tr.basis.rows[2][1] = p_array[9 + p_ofs];
	tr.basis.rows[2][2] = p_array[10 + p_ofs];
	tr.origin.x = p_array[3 + p_ofs];
	tr.origin.y = p_array[7 + p_ofs];
	tr.origin.z = p_array[11 + p_ofs];
	return tr;
}

/* STRUCTURES */

Transform3D Collada::Node::compute_transform(const Collada& p_state) const
{
	Transform3D xform;

	for (int i = 0; i < xform_list.size(); i++) {
		Transform3D xform_step;
		const XForm& xf = xform_list[i];
		switch (xf.op) {
		case XForm::OP_ROTATE: {
			if (xf.data.size() >= 4) {
				xform_step.rotate(
					Vector3(xf.data[0], xf.data[1], xf.data[2]), Math::deg_to_rad(xf.data[3]));
			}
		} break;
		case XForm::OP_SCALE: {
			if (xf.data.size() >= 3) {
				xform_step.scale(Vector3(xf.data[0], xf.data[1], xf.data[2]));
			}

		} break;
		case XForm::OP_TRANSLATE: {
			if (xf.data.size() >= 3) {
				xform_step.origin = Vector3(xf.data[0], xf.data[1], xf.data[2]);
			}

		} break;
		case XForm::OP_MATRIX: {
			if (xf.data.size() >= 16) {
				xform_step = _read_transform_from_array(xf.data, 0);
			}

		} break;
		default: {
		}
		}

		xform = xform * xform_step;
	}

#ifdef COLLADA_IMPORT_SCALE_SCENE
	xform.origin *= p_state.state.unit_scale;
#endif
	return xform;
}

Transform3D Collada::Node::get_transform() const { return default_transform; }

Transform3D Collada::Node::get_global_transform() const
{
	if (parent) {
		return parent->get_global_transform() * default_transform;
	}
	else {
		return default_transform;
	}
}

Vector<float> Collada::AnimationTrack::get_value_at_time(float p_time) const
{
	ERR_FAIL_COND_V(keys.is_empty(), Vector<float>());
	int i = 0;

	for (i = 0; i < keys.size(); i++) {
		if (keys[i].time > p_time) {
			break;
		}
	}

	if (i == 0) {
		return keys[0].data;
	}
	if (i == keys.size()) {
		return keys[keys.size() - 1].data;
	}

	switch (keys[i].interp_type) {
	case INTERP_BEZIER: // wait for bezier
	case INTERP_LINEAR: {
		float c = (p_time - keys[i - 1].time) / (keys[i].time - keys[i - 1].time);

		if (keys[i].data.size() == 16) {
			// interpolate a matrix
			Transform3D src = _read_transform_from_array(keys[i - 1].data);
			Transform3D dst = _read_transform_from_array(keys[i].data);

			Transform3D interp = c < 0.001 ? src : src.interpolate_with(dst, c);

			Vector<float> ret;
			ret.resize(16);
			// i wonder why collada matrices are transposed, given that's opposed to opengl..
			ret.write[0] = interp.basis.rows[0][0];
			ret.write[1] = interp.basis.rows[0][1];
			ret.write[2] = interp.basis.rows[0][2];
			ret.write[4] = interp.basis.rows[1][0];
			ret.write[5] = interp.basis.rows[1][1];
			ret.write[6] = interp.basis.rows[1][2];
			ret.write[8] = interp.basis.rows[2][0];
			ret.write[9] = interp.basis.rows[2][1];
			ret.write[10] = interp.basis.rows[2][2];
			ret.write[3] = interp.origin.x;
			ret.write[7] = interp.origin.y;
			ret.write[11] = interp.origin.z;
			ret.write[12] = 0;
			ret.write[13] = 0;
			ret.write[14] = 0;
			ret.write[15] = 1;

			return ret;
		}
		else {
			Vector<float> dest;
			dest.resize(keys[i].data.size());
			for (int j = 0; j < dest.size(); j++) {
				dest.write[j] = keys[i].data[j] * c + keys[i - 1].data[j] * (1.0 - c);
			}
			return dest;
			// interpolate one by one
		}
	} break;
	}

	ERR_FAIL_V(Vector<float>());
}

void Collada::_parse_asset(XMLParser& p_parser)
{
	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();

			if (name == "up_axis") {
				p_parser.read();
				if (p_parser.get_node_data() == "X_UP") {
					state.up_axis = Vector3::AXIS_X;
				}
				if (p_parser.get_node_data() == "Y_UP") {
					state.up_axis = Vector3::AXIS_Y;
				}
				if (p_parser.get_node_data() == "Z_UP") {
					state.up_axis = Vector3::AXIS_Z;
				}

				COLLADA_PRINT("up axis: " + p_parser.get_node_data());
			}
			else if (name == "unit") {
				state.unit_scale = p_parser.get_named_attribute_value("meter").to_float();
				COLLADA_PRINT("unit scale: " + rtos(state.unit_scale));
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "asset") {
			break; // end of <asset>
		}
	}
}

void Collada::_parse_image(XMLParser& p_parser)
{
	String id = p_parser.get_named_attribute_value("id");

	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	Image image;

	if (state.version < State::Version(1, 4, 0)) {
		/* <1.4 */
		String path = p_parser.get_named_attribute_value("source").strip_edges();
		if (!path.contains("://") && path.is_relative_path()) {
			// path is relative to file being loaded, so convert to a resource path
			image.path = ProjectSettings::localize_path(
				state.local_path.get_base_dir().path_join(path.uri_file_decode()));
		}
	}
	else {
		while (p_parser.read() == OK) {
			if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
				String name = p_parser.get_node_name();

				if (name == "init_from") {
					p_parser.read();
					String path = p_parser.get_node_data().strip_edges().uri_file_decode();

					if (!path.contains("://") && path.is_relative_path()) {
						// path is relative to file being loaded, so convert to a resource path
						path = ProjectSettings::localize_path(
							state.local_path.get_base_dir().path_join(path));

					}
					else if (path.find("file:///") == 0) {
						path = path.replace_first("file:///", "");
						path = ProjectSettings::localize_path(path);
					}

					image.path = path;

				}
				else if (name == "data") {
					ERR_PRINT("COLLADA Embedded image data not supported!");

				}
				else if (name == "extra" && !p_parser.is_empty()) {
					p_parser.skip_section();
				}

			}
			else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
					   p_parser.get_node_name() == "image") {
				break; // end of <asset>
			}
		}
	}

	state.image_map[id] = image;
}

void Collada::_parse_material(XMLParser& p_parser)
{
	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	Material material;

	String id = p_parser.get_named_attribute_value("id");
	if (p_parser.has_attribute("name")) {
		material.name = p_parser.get_named_attribute_value("name");
	}

	if (state.version < State::Version(1, 4, 0)) {
		/* <1.4 */
		ERR_PRINT("Collada Materials < 1.4 are not supported (yet)");
	}
	else {
		while (p_parser.read() == OK) {
			if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT &&
				p_parser.get_node_name() == "instance_effect") {
				material.instance_effect = _uri_to_id(p_parser.get_named_attribute_value("url"));
			}
			else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
					   p_parser.get_node_name() == "material") {
				break; // end of <asset>
			}
		}
	}

	state.material_map[id] = material;
}

//! reads floats from inside of xml element until end of xml element
Vector<float> Collada::_read_float_array(XMLParser& p_parser)
{
	if (p_parser.is_empty()) {
		return Vector<float>();
	}

	Vector<String> splitters;
	splitters.push_back(" ");
	splitters.push_back("\n");
	splitters.push_back("\r");
	splitters.push_back("\t");

	Vector<float> array;
	while (p_parser.read() == OK) {
		// TODO: check for comments inside the element
		// and ignore them.

		if (p_parser.get_node_type() == XMLParser::NODE_TEXT) {
			// parse float data
			String str = p_parser.get_node_data();
			array = str.split_floats_mk(splitters, false);
			// array=str.split_floats(" ",false);
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END) {
			break; // end parsing text
		}
	}

	return array;
}

Vector<String> Collada::_read_string_array(XMLParser& p_parser)
{
	if (p_parser.is_empty()) {
		return Vector<String>();
	}

	Vector<String> array;
	while (p_parser.read() == OK) {
		// TODO: check for comments inside the element
		// and ignore them.

		if (p_parser.get_node_type() == XMLParser::NODE_TEXT) {
			// parse String data
			String str = p_parser.get_node_data();
			array = str.split_spaces();
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END) {
			break; // end parsing text
		}
	}

	return array;
}

Transform3D Collada::_read_transform(XMLParser& p_parser)
{
	if (p_parser.is_empty()) {
		return Transform3D();
	}

	Vector<String> array;
	while (p_parser.read() == OK) {
		// TODO: check for comments inside the element
		// and ignore them.

		if (p_parser.get_node_type() == XMLParser::NODE_TEXT) {
			// parse float data
			String str = p_parser.get_node_data();
			array = str.split_spaces();
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END) {
			break; // end parsing text
		}
	}

	ERR_FAIL_COND_V(array.size() != 16, Transform3D());
	Vector<float> farr;
	farr.resize(16);
	for (int i = 0; i < 16; i++) {
		farr.write[i] = array[i].to_float();
	}

	return _read_transform_from_array(farr);
}

String Collada::_read_empty_draw_type(XMLParser& p_parser)
{
	String empty_draw_type = "";

	if (p_parser.is_empty()) {
		return empty_draw_type;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_TEXT) {
			empty_draw_type = p_parser.get_node_data();
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END) {
			break; // end parsing text
		}
	}
	return empty_draw_type;
}

void Collada::_parse_effect(XMLParser& p_parser)
{
	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	String id = p_parser.get_named_attribute_value("id");

	Effect effect;
	if (p_parser.has_attribute("name")) {
		effect.name = p_parser.get_named_attribute_value("name");
	}
	_parse_effect_material(p_parser, effect, id);

	state.effect_map[id] = effect;

	COLLADA_PRINT("Effect ID:" + id);
}

void Collada::_parse_camera(XMLParser& p_parser)
{
	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	String id = p_parser.get_named_attribute_value("id");

	state.camera_data_map[id] = CameraData();
	CameraData& camera = state.camera_data_map[id];

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();

			if (name == "perspective") {
				camera.mode = CameraData::MODE_PERSPECTIVE;
			}
			else if (name == "orthographic") {
				camera.mode = CameraData::MODE_ORTHOGONAL;
			}
			else if (name == "xfov") {
				p_parser.read();
				camera.perspective.x_fov = p_parser.get_node_data().to_float();

			}
			else if (name == "yfov") {
				p_parser.read();
				camera.perspective.y_fov = p_parser.get_node_data().to_float();
			}
			else if (name == "xmag") {
				p_parser.read();
				camera.orthogonal.x_mag = p_parser.get_node_data().to_float();

			}
			else if (name == "ymag") {
				p_parser.read();
				camera.orthogonal.y_mag = p_parser.get_node_data().to_float();
			}
			else if (name == "aspect_ratio") {
				p_parser.read();
				camera.aspect = p_parser.get_node_data().to_float();

			}
			else if (name == "znear") {
				p_parser.read();
				camera.z_near = p_parser.get_node_data().to_float();

			}
			else if (name == "zfar") {
				p_parser.read();
				camera.z_far = p_parser.get_node_data().to_float();
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "camera") {
			break; // end of <asset>
		}
	}

	COLLADA_PRINT("Camera ID:" + id);
}

void Collada::_parse_light(XMLParser& p_parser)
{
	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	String id = p_parser.get_named_attribute_value("id");

	state.light_data_map[id] = LightData();
	LightData& light = state.light_data_map[id];

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();

			if (name == "ambient") {
				light.mode = LightData::MODE_AMBIENT;
			}
			else if (name == "directional") {
				light.mode = LightData::MODE_DIRECTIONAL;
			}
			else if (name == "point") {
				light.mode = LightData::MODE_OMNI;
			}
			else if (name == "spot") {
				light.mode = LightData::MODE_SPOT;
			}
			else if (name == "color") {
				p_parser.read();
				Vector<float> colorarr = _read_float_array(p_parser);
				COLLADA_PRINT("colorarr size: " + rtos(colorarr.size()));

				if (colorarr.size() >= 4) {
					// alpha strangely not alright? maybe it needs to be multiplied by value as a
					// channel intensity
					Color color(colorarr[0], colorarr[1], colorarr[2], 1.0);
					light.color = color;
				}

			}
			else if (name == "constant_attenuation") {
				p_parser.read();
				light.constant_att = p_parser.get_node_data().to_float();
			}
			else if (name == "linear_attenuation") {
				p_parser.read();
				light.linear_att = p_parser.get_node_data().to_float();
			}
			else if (name == "quadratic_attenuation") {
				p_parser.read();
				light.quad_att = p_parser.get_node_data().to_float();
			}
			else if (name == "falloff_angle") {
				p_parser.read();
				light.spot_angle = p_parser.get_node_data().to_float();

			}
			else if (name == "falloff_exponent") {
				p_parser.read();
				light.spot_exp = p_parser.get_node_data().to_float();
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "light") {
			break; // end of <asset>
		}
	}

	COLLADA_PRINT("Light ID:" + id);
}

void Collada::_parse_curve_geometry(XMLParser& p_parser, const String& p_id, const String& p_name)
{
	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	// load everything into a pre dictionary

	state.curve_data_map[p_id] = CurveData();

	CurveData& curvedata = state.curve_data_map[p_id];
	curvedata.name = p_name;
	String closed = p_parser.get_named_attribute_value_safe("closed").to_lower();
	curvedata.closed = closed == "true" || closed == "1";

	COLLADA_PRINT("curve name: " + p_name);

	String current_source;
	// handles geometry node and the curve children in this loop
	// read sources with arrays and accessor for each curve
	if (p_parser.is_empty()) {
		return;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "source") {
				String id = p_parser.get_named_attribute_value("id");
				curvedata.sources[id] = CurveData::Source();
				current_source = id;
				COLLADA_PRINT("source data: " + id);

			}
			else if (section == "float_array" || section == "array") {
				// create a new array and read it.
				if (curvedata.sources.has(current_source)) {
					curvedata.sources[current_source].array = _read_float_array(p_parser);
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(curvedata.sources[current_source].array.size()) +
								  " values.");
				}
			}
			else if (section == "Name_array") {
				// create a new array and read it.
				if (curvedata.sources.has(current_source)) {
					curvedata.sources[current_source].sarray = _read_string_array(p_parser);
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(curvedata.sources[current_source].array.size()) +
								  " values.");
				}

			}
			else if (section == "technique_common") {
				// skip it
			}
			else if (section == "accessor") { // child of source (below a technique tag)

				if (curvedata.sources.has(current_source)) {
					curvedata.sources[current_source].stride =
						p_parser.get_named_attribute_value("stride").to_int();
					COLLADA_PRINT("section: " + current_source + " stride " +
								  itos(curvedata.sources[current_source].stride));
				}
			}
			else if (section == "control_vertices") {
				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "input") {
							String semantic = p_parser.get_named_attribute_value("semantic");
							String source =
								_uri_to_id(p_parser.get_named_attribute_value("source"));

							curvedata.control_vertices[semantic] = source;

							COLLADA_PRINT(
								section + " input semantic: " + semantic + " source: " + source);
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == section) {
						break;
					}
				}

			}
			else if (!p_parser.is_empty()) {
				p_parser.skip_section();
			}
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "spline") {
			break;
		}
	}
}

void Collada::_parse_mesh_geometry(XMLParser& p_parser, const String& p_id, const String& p_name)
{
	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	// load everything into a pre dictionary

	state.mesh_data_map[p_id] = MeshData();

	MeshData& meshdata = state.mesh_data_map[p_id];
	meshdata.name = p_name;

	COLLADA_PRINT("mesh name: " + p_name);

	String current_source;
	// handles geometry node and the mesh children in this loop
	// read sources with arrays and accessor for each mesh
	if (p_parser.is_empty()) {
		return;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "source") {
				String id = p_parser.get_named_attribute_value("id");
				meshdata.sources[id] = MeshData::Source();
				current_source = id;
				COLLADA_PRINT("source data: " + id);

			}
			else if (section == "float_array" || section == "array") {
				// create a new array and read it.
				if (meshdata.sources.has(current_source)) {
					meshdata.sources[current_source].array = _read_float_array(p_parser);
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(meshdata.sources[current_source].array.size()) + " values.");
				}
			}
			else if (section == "technique_common") {
				// skip it
			}
			else if (section == "accessor") { // child of source (below a technique tag)

				if (meshdata.sources.has(current_source)) {
					meshdata.sources[current_source].stride =
						p_parser.get_named_attribute_value("stride").to_int();
					COLLADA_PRINT("section: " + current_source + " stride " +
								  itos(meshdata.sources[current_source].stride));
				}
			}
			else if (section == "vertices") {
				MeshData::Vertices vert;
				String id = p_parser.get_named_attribute_value("id");
				int last_ref = 0;

				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "input") {
							String semantic = p_parser.get_named_attribute_value("semantic");
							String source =
								_uri_to_id(p_parser.get_named_attribute_value("source"));

							if (semantic == "TEXCOORD") {
								semantic = "TEXCOORD" + itos(last_ref++);
							}

							vert.sources[semantic] = source;

							COLLADA_PRINT(
								section + " input semantic: " + semantic + " source: " + source);
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == section) {
						break;
					}
				}

				meshdata.vertices[id] = vert;

			}
			else if (section == "triangles" || section == "polylist" || section == "polygons") {
				bool polygons = (section == "polygons");
				if (polygons) {
					WARN_PRINT("Primitive type \"polygons\" is not well supported (concave shapes "
							   "may fail). To ensure that the geometry is properly imported, "
							   "please re-export using \"triangles\" or \"polylist\".");
				}
				MeshData::Primitives prim;

				if (p_parser.has_attribute("material")) {
					prim.material = p_parser.get_named_attribute_value("material");
				}
				prim.count = p_parser.get_named_attribute_value("count").to_int();
				prim.vertex_size = 0;
				int last_ref = 0;

				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "input") {
							String semantic = p_parser.get_named_attribute_value("semantic");
							String source =
								_uri_to_id(p_parser.get_named_attribute_value("source"));

							if (semantic == "TEXCOORD") {
								semantic = "TEXCOORD" + itos(last_ref++);
							}
							int offset = p_parser.get_named_attribute_value("offset").to_int();

							MeshData::Primitives::SourceRef sref;
							sref.source = source;
							sref.offset = offset;
							prim.sources[semantic] = sref;
							prim.vertex_size = MAX(prim.vertex_size, offset + 1);

							COLLADA_PRINT(section + " input semantic: " + semantic +
										  " source: " + source + " offset: " + itos(offset));

						}
						else if (p_parser.get_node_name() == "p") { // indices

							Vector<float> values = _read_float_array(p_parser);
							if (polygons) {
								ERR_CONTINUE(prim.vertex_size == 0);
								prim.polygons.push_back(values.size() / prim.vertex_size);
								int from = prim.indices.size();
								prim.indices.resize(from + values.size());
								for (int i = 0; i < values.size(); i++) {
									prim.indices.write[from + i] = values[i];
								}

							}
							else if (prim.vertex_size > 0) {
								prim.indices = values;
							}

							COLLADA_PRINT("read " + itos(values.size()) + " index values");

						}
						else if (p_parser.get_node_name() == "vcount") { // primitive

							Vector<float> values = _read_float_array(p_parser);
							prim.polygons = values;
							COLLADA_PRINT("read " + itos(values.size()) + " polygon values");
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == section) {
						break;
					}
				}

				meshdata.primitives.push_back(prim);

			}
			else if (p_parser.get_node_name() == "double_sided") {
				p_parser.read();
				meshdata.found_double_sided = true;
				meshdata.double_sided = p_parser.get_node_data().to_int();

			}
			else if (p_parser.get_node_name() == "polygons") {
				ERR_PRINT("Primitive type \"polygons\" not supported, re-export using \"polylist\" "
						  "or \"triangles\".");
			}
			else if (!p_parser.is_empty()) {
				p_parser.skip_section();
			}
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "mesh") {
			break;
		}
	}
}

void Collada::_parse_skin_controller(XMLParser& p_parser, const String& p_id)
{
	state.skin_controller_data_map[p_id] = SkinControllerData();
	SkinControllerData& skindata = state.skin_controller_data_map[p_id];

	skindata.base = _uri_to_id(p_parser.get_named_attribute_value("source"));

	String current_source;

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "bind_shape_matrix") {
				skindata.bind_shape = _read_transform(p_parser);
#ifdef COLLADA_IMPORT_SCALE_SCENE
				skindata.bind_shape.origin *= state.unit_scale;

#endif
				COLLADA_PRINT("skeleton bind shape transform: " + skindata.bind_shape);

			}
			else if (section == "source") {
				String id = p_parser.get_named_attribute_value("id");
				skindata.sources[id] = SkinControllerData::Source();
				current_source = id;
				COLLADA_PRINT("source data: " + id);

			}
			else if (section == "float_array" || section == "array") {
				// create a new array and read it.
				if (skindata.sources.has(current_source)) {
					skindata.sources[current_source].array = _read_float_array(p_parser);
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(skindata.sources[current_source].array.size()) + " values.");
				}
			}
			else if (section == "Name_array" || section == "IDREF_array") {
				// create a new array and read it.

				if (section == "IDREF_array") {
					skindata.use_idrefs = true;
				}
				if (skindata.sources.has(current_source)) {
					skindata.sources[current_source].sarray = _read_string_array(p_parser);
					if (section == "IDREF_array") {
						Vector<String> sa = skindata.sources[current_source].sarray;
						for (int i = 0; i < sa.size(); i++) {
							state.idref_joints.insert(sa[i]);
						}
					}
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(skindata.sources[current_source].array.size()) + " values.");
				}
			}
			else if (section == "technique_common") {
				// skip it
			}
			else if (section == "accessor") { // child of source (below a technique tag)

				if (skindata.sources.has(current_source)) {
					int stride = 1;
					if (p_parser.has_attribute("stride")) {
						stride = p_parser.get_named_attribute_value("stride").to_int();
					}

					skindata.sources[current_source].stride = stride;
					COLLADA_PRINT("section: " + current_source + " stride " +
								  itos(skindata.sources[current_source].stride));
				}

			}
			else if (section == "joints") {
				SkinControllerData::Joints joint;

				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "input") {
							String semantic = p_parser.get_named_attribute_value("semantic");
							String source =
								_uri_to_id(p_parser.get_named_attribute_value("source"));

							joint.sources[semantic] = source;

							COLLADA_PRINT(
								section + " input semantic: " + semantic + " source: " + source);
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == section) {
						break;
					}
				}

				skindata.joints = joint;

			}
			else if (section == "vertex_weights") {
				SkinControllerData::Weights weights;

				weights.count = p_parser.get_named_attribute_value("count").to_int();

				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "input") {
							String semantic = p_parser.get_named_attribute_value("semantic");
							String source =
								_uri_to_id(p_parser.get_named_attribute_value("source"));

							int offset = p_parser.get_named_attribute_value("offset").to_int();

							SkinControllerData::Weights::SourceRef sref;
							sref.source = source;
							sref.offset = offset;
							weights.sources[semantic] = sref;

							COLLADA_PRINT(section + " input semantic: " + semantic +
										  " source: " + source + " offset: " + itos(offset));

						}
						else if (p_parser.get_node_name() == "v") { // indices

							Vector<float> values = _read_float_array(p_parser);
							weights.indices = values;
							COLLADA_PRINT("read " + itos(values.size()) + " index values");

						}
						else if (p_parser.get_node_name() == "vcount") { // weightsitive

							Vector<float> values = _read_float_array(p_parser);
							weights.sets = values;
							COLLADA_PRINT("read " + itos(values.size()) + " polygon values");
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == section) {
						break;
					}
				}

				skindata.weights = weights;
			}
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "skin") {
			break;
		}
	}

	/* STORE REST MATRICES */

	ERR_FAIL_COND(!skindata.joints.sources.has("JOINT"));
	ERR_FAIL_COND(!skindata.joints.sources.has("INV_BIND_MATRIX"));

	String joint_arr = skindata.joints.sources["JOINT"];
	String ibm = skindata.joints.sources["INV_BIND_MATRIX"];

	ERR_FAIL_COND(!skindata.sources.has(joint_arr));
	ERR_FAIL_COND(!skindata.sources.has(ibm));

	SkinControllerData::Source& joint_source = skindata.sources[joint_arr];
	SkinControllerData::Source& ibm_source = skindata.sources[ibm];

	ERR_FAIL_COND(joint_source.sarray.size() != ibm_source.array.size() / 16);

	for (int i = 0; i < joint_source.sarray.size(); i++) {
		String name = joint_source.sarray[i];
		Transform3D xform = _read_transform_from_array(
			ibm_source.array, i * 16); //<- this is a mistake, it must be applied to vertices
		xform.affine_invert();		   // inverse for rest, because it's an inverse
#ifdef COLLADA_IMPORT_SCALE_SCENE
		xform.origin *= state.unit_scale;
#endif
		skindata.bone_rest_map[name] = xform;
	}
}

void Collada::_parse_morph_controller(XMLParser& p_parser, const String& p_id)
{
	state.morph_controller_data_map[p_id] = MorphControllerData();
	MorphControllerData& morphdata = state.morph_controller_data_map[p_id];

	morphdata.mesh = _uri_to_id(p_parser.get_named_attribute_value("source"));
	morphdata.mode = p_parser.get_named_attribute_value("method");
	String current_source;

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "source") {
				String id = p_parser.get_named_attribute_value("id");
				morphdata.sources[id] = MorphControllerData::Source();
				current_source = id;
				COLLADA_PRINT("source data: " + id);

			}
			else if (section == "float_array" || section == "array") {
				// create a new array and read it.
				if (morphdata.sources.has(current_source)) {
					morphdata.sources[current_source].array = _read_float_array(p_parser);
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(morphdata.sources[current_source].array.size()) +
								  " values.");
				}
			}
			else if (section == "Name_array" || section == "IDREF_array") {
				// create a new array and read it.
				if (morphdata.sources.has(current_source)) {
					morphdata.sources[current_source].sarray = _read_string_array(p_parser);
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(morphdata.sources[current_source].array.size()) +
								  " values.");
				}
			}
			else if (section == "technique_common") {
				// skip it
			}
			else if (section == "accessor") { // child of source (below a technique tag)

				if (morphdata.sources.has(current_source)) {
					int stride = 1;
					if (p_parser.has_attribute("stride")) {
						stride = p_parser.get_named_attribute_value("stride").to_int();
					}

					morphdata.sources[current_source].stride = stride;
					COLLADA_PRINT("section: " + current_source + " stride " +
								  itos(morphdata.sources[current_source].stride));
				}

			}
			else if (section == "targets") {
				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "input") {
							String semantic = p_parser.get_named_attribute_value("semantic");
							String source =
								_uri_to_id(p_parser.get_named_attribute_value("source"));

							morphdata.targets[semantic] = source;

							COLLADA_PRINT(
								section + " input semantic: " + semantic + " source: " + source);
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == section) {
						break;
					}
				}
			}
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "morph") {
			break;
		}
	}

	if (morphdata.targets.has("MORPH_WEIGHT")) {
		state.morph_name_map[morphdata.targets["MORPH_WEIGHT"]] = p_id;
	}
}

void Collada::_parse_controller(XMLParser& p_parser)
{
	String id = p_parser.get_named_attribute_value("id");

	if (p_parser.is_empty()) {
		return;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "skin") {
				_parse_skin_controller(p_parser, id);
			}
			else if (section == "morph") {
				_parse_morph_controller(p_parser, id);
			}
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "controller") {
			break;
		}
	}
}

Collada::Node* Collada::_parse_visual_instance_geometry(XMLParser& p_parser)
{
	String type = p_parser.get_node_name();
	NodeGeometry* geom = memnew(NodeGeometry);
	geom->controller = type == "instance_controller";
	geom->source = _uri_to_id(p_parser.get_named_attribute_value_safe("url"));

	if (p_parser.is_empty()) { // nothing else to parse...
		return geom;
	}
	// try to find also many materials and skeletons!
	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			if (p_parser.get_node_name() == "instance_material") {
				String symbol = p_parser.get_named_attribute_value("symbol");
				String target = _uri_to_id(p_parser.get_named_attribute_value("target"));

				NodeGeometry::Material mat;
				mat.target = target;
				geom->material_map[symbol] = mat;
				COLLADA_PRINT("uses material: '" + target + "' on primitive'" + symbol + "'");
			}
			else if (p_parser.get_node_name() == "skeleton") {
				p_parser.read();
				String uri = _uri_to_id(p_parser.get_node_data());
				if (!uri.is_empty()) {
					geom->skeletons.push_back(uri);
				}
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == type) {
			break;
		}
	}

	if (geom->controller) {
		if (geom->skeletons.is_empty()) {
			// XSI style

			if (state.skin_controller_data_map.has(geom->source)) {
				SkinControllerData* skin = &state.skin_controller_data_map[geom->source];
				// case where skeletons reference bones with IDREF (XSI)
				ERR_FAIL_COND_V(!skin->joints.sources.has("JOINT"), geom);
				String joint_arr = skin->joints.sources["JOINT"];
				ERR_FAIL_COND_V(!skin->sources.has(joint_arr), geom);
				Collada::SkinControllerData::Source& joint_source = skin->sources[joint_arr];
				geom->skeletons = joint_source.sarray; // quite crazy, but should work.
			}
		}
	}

	return geom;
}

Collada::Node* Collada::_parse_visual_instance_camera(XMLParser& p_parser)
{
	NodeCamera* cam = memnew(NodeCamera);
	cam->camera = _uri_to_id(p_parser.get_named_attribute_value_safe("url"));

	if (state.up_axis == Vector3::AXIS_Z) { // collada weirdness
		cam->post_transform.basis.rotate(Vector3(1, 0, 0), -Math::PI * 0.5);
	}

	if (p_parser.is_empty()) { // nothing else to parse...
		return cam;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
			p_parser.get_node_name() == "instance_camera") {
			break;
		}
	}

	return cam;
}

Collada::Node* Collada::_parse_visual_instance_light(XMLParser& p_parser)
{
	NodeLight* cam = memnew(NodeLight);
	cam->light = _uri_to_id(p_parser.get_named_attribute_value_safe("url"));

	if (state.up_axis == Vector3::AXIS_Z) { // collada weirdness
		cam->post_transform.basis.rotate(Vector3(1, 0, 0), -Math::PI * 0.5);
	}

	if (p_parser.is_empty()) { // nothing else to parse...
		return cam;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
			p_parser.get_node_name() == "instance_light") {
			break;
		}
	}

	return cam;
}

Collada::Node* Collada::_parse_visual_node_instance_data(XMLParser& p_parser)
{
	String instance_type = p_parser.get_node_name();

	if (instance_type == "instance_geometry" || instance_type == "instance_controller") {
		return _parse_visual_instance_geometry(p_parser);
	}
	else if (instance_type == "instance_camera") {
		return _parse_visual_instance_camera(p_parser);
	}
	else if (instance_type == "instance_light") {
		return _parse_visual_instance_light(p_parser);
	}

	if (p_parser.is_empty()) { // nothing else to parse...
		return nullptr;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
			p_parser.get_node_name() == instance_type) {
			break;
		}
	}

	return nullptr;
}

Collada::Node* Collada::_parse_visual_scene_node(XMLParser& p_parser)
{
	String name;

	String id = p_parser.get_named_attribute_value_safe("id");

	bool found_name = false;

	if (id.is_empty()) {
		id = "%NODEID%" + itos(Math::rand());

	}
	else {
		found_name = true;
	}

	Vector<Node::XForm> xform_list;
	Vector<Node*> children;

	String empty_draw_type = "";

	Node* node = nullptr;

	name = p_parser.has_attribute("name") ? p_parser.get_named_attribute_value_safe("name")
										  : p_parser.get_named_attribute_value_safe("id");
	if (name.is_empty()) {
		name = id;
	}
	else {
		found_name = true;
	}

	if ((p_parser.has_attribute("type") && p_parser.get_named_attribute_value("type") == "JOINT") ||
		state.idref_joints.has(name)) {
		// handle a bone

		NodeJoint* joint = memnew(NodeJoint);

		if (p_parser.has_attribute("sid")) { // bones may not have sid
			joint->sid = p_parser.get_named_attribute_value("sid");
			// state.bone_map[joint->sid]=joint;
		}
		else if (state.idref_joints.has(name)) {
			joint->sid = name; // kind of a cheat but..
		}
		else if (p_parser.has_attribute("name")) {
			joint->sid = p_parser.get_named_attribute_value_safe("name");
		}

		if (!joint->sid.is_empty()) {
			state.sid_to_node_map[joint->sid] = id;
		}

		node = joint;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "translate") {
				Node::XForm xf;
				if (p_parser.has_attribute("sid")) {
					xf.id = p_parser.get_named_attribute_value("sid");
				}
				xf.op = Node::XForm::OP_TRANSLATE;

				Vector<float> xlt = _read_float_array(p_parser);
				xf.data = xlt;
				xform_list.push_back(xf);

			}
			else if (section == "rotate") {
				Node::XForm xf;
				if (p_parser.has_attribute("sid")) {
					xf.id = p_parser.get_named_attribute_value("sid");
				}
				xf.op = Node::XForm::OP_ROTATE;

				Vector<float> rot = _read_float_array(p_parser);
				xf.data = rot;

				xform_list.push_back(xf);

			}
			else if (section == "scale") {
				Node::XForm xf;
				if (p_parser.has_attribute("sid")) {
					xf.id = p_parser.get_named_attribute_value("sid");
				}

				xf.op = Node::XForm::OP_SCALE;

				Vector<float> scale = _read_float_array(p_parser);

				xf.data = scale;

				xform_list.push_back(xf);

			}
			else if (section == "matrix") {
				Node::XForm xf;
				if (p_parser.has_attribute("sid")) {
					xf.id = p_parser.get_named_attribute_value("sid");
				}
				xf.op = Node::XForm::OP_MATRIX;

				Vector<float> matrix = _read_float_array(p_parser);

				xf.data = matrix;
				String mtx;
				for (int i = 0; i < matrix.size(); i++) {
					mtx += " " + rtos(matrix[i]);
				}

				xform_list.push_back(xf);

			}
			else if (section == "visibility") {
				Node::XForm xf;
				if (p_parser.has_attribute("sid")) {
					xf.id = p_parser.get_named_attribute_value("sid");
				}
				xf.op = Node::XForm::OP_VISIBILITY;

				Vector<float> visible = _read_float_array(p_parser);

				xf.data = visible;

				xform_list.push_back(xf);

			}
			else if (section == "empty_draw_type") {
				empty_draw_type = _read_empty_draw_type(p_parser);
			}
			else if (section == "technique" || section == "extra") {
			}
			else if (section != "node") {
				// usually what defines the type of node
				if (section.begins_with("instance_")) {
					if (!node) {
						node = _parse_visual_node_instance_data(p_parser);

					}
					else {
						ERR_PRINT("Multiple instance_* not supported.");
					}
				}

			}
			else {
				/* Found a child node!! what to do..*/

				Node* child = _parse_visual_scene_node(p_parser);
				children.push_back(child);
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "node") {
			break;
		}
	}

	if (!node) {
		node = memnew(Node); // generic node, nothing of relevance found
	}

	node->noname = !found_name;
	node->xform_list = xform_list;
	node->children = children;
	for (int i = 0; i < children.size(); i++) {
		node->children[i]->parent = node;
	}

	node->name = name;
	node->id = id;
	node->empty_draw_type = empty_draw_type;

	if (node->children.size() == 1) {
		if (node->children[0]->noname && !node->noname) {
			node->children[0]->name = node->name;
			node->name = node->name + "-base";
		}
	}

	node->default_transform = node->compute_transform(*this);
	state.scene_map[id] = node;

	return node;
}

void Collada::_parse_visual_scene(XMLParser& p_parser)
{
	String id = p_parser.get_named_attribute_value("id");

	if (p_parser.is_empty()) {
		return;
	}

	state.visual_scene_map[id] = VisualScene();
	VisualScene& vscene = state.visual_scene_map[id];

	if (p_parser.has_attribute("name")) {
		vscene.name = p_parser.get_named_attribute_value("name");
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "node") {
				vscene.root_nodes.push_back(_parse_visual_scene_node(p_parser));
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "visual_scene") {
			break;
		}
	}

	COLLADA_PRINT("Scene ID:" + id);
}

void Collada::_parse_animation(XMLParser& p_parser)
{
	if (!(state.import_flags & IMPORT_FLAG_ANIMATION)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}

		return;
	}

	HashMap<String, Vector<float>> float_sources;
	HashMap<String, Vector<String>> string_sources;
	HashMap<String, int> source_strides;
	HashMap<String, HashMap<String, String>> samplers;
	HashMap<String, Vector<String>> source_param_names;
	HashMap<String, Vector<String>> source_param_types;

	String id = "";
	if (p_parser.has_attribute("id")) {
		id = p_parser.get_named_attribute_value("id");
	}

	String current_source;
	String current_sampler;
	Vector<String> channel_sources;
	Vector<String> channel_targets;

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();
			if (name == "source") {
				current_source = p_parser.get_named_attribute_value("id");
				source_param_names[current_source] = Vector<String>();
				source_param_types[current_source] = Vector<String>();

			}
			else if (name == "float_array") {
				if (!current_source.is_empty()) {
					float_sources[current_source] = _read_float_array(p_parser);
				}

			}
			else if (name == "Name_array") {
				if (!current_source.is_empty()) {
					string_sources[current_source] = _read_string_array(p_parser);
				}
			}
			else if (name == "accessor") {
				if (!current_source.is_empty() && p_parser.has_attribute("stride")) {
					source_strides[current_source] =
						p_parser.get_named_attribute_value("stride").to_int();
				}
			}
			else if (name == "sampler") {
				current_sampler = p_parser.get_named_attribute_value("id");
				samplers[current_sampler] = HashMap<String, String>();
			}
			else if (name == "param") {
				if (p_parser.has_attribute("name")) {
					source_param_names[current_source].push_back(
						p_parser.get_named_attribute_value("name"));
				}
				else {
					source_param_names[current_source].push_back("");
				}

				if (p_parser.has_attribute("type")) {
					source_param_types[current_source].push_back(
						p_parser.get_named_attribute_value("type"));
				}
				else {
					source_param_types[current_source].push_back("");
				}

			}
			else if (name == "input") {
				if (!current_sampler.is_empty()) {
					samplers[current_sampler][p_parser.get_named_attribute_value("semantic")] =
						p_parser.get_named_attribute_value("source");
				}

			}
			else if (name == "channel") {
				channel_sources.push_back(p_parser.get_named_attribute_value("source"));
				channel_targets.push_back(p_parser.get_named_attribute_value("target"));
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "animation") {
			break; // end of <asset>
		}
	}

	for (int i = 0; i < channel_sources.size(); i++) {
		String source = _uri_to_id(channel_sources[i]);
		const String& target = channel_targets[i];
		ERR_CONTINUE(!samplers.has(source));
		HashMap<String, String>& sampler = samplers[source];

		ERR_CONTINUE(!sampler.has("INPUT")); // no input semantic? wtf?
		String input_id = _uri_to_id(sampler["INPUT"]);
		COLLADA_PRINT("input id is " + input_id);
		ERR_CONTINUE(!float_sources.has(input_id));

		ERR_CONTINUE(!sampler.has("OUTPUT"));
		String output_id = _uri_to_id(sampler["OUTPUT"]);
		ERR_CONTINUE(!float_sources.has(output_id));

		ERR_CONTINUE(!source_param_names.has(output_id));

		Vector<String>& names = source_param_names[output_id];

		for (int l = 0; l < names.size(); l++) {
			String name = names[l];

			Vector<float>& time_keys = float_sources[input_id];
			int key_count = time_keys.size();

			AnimationTrack track; // begin crating track
			track.id = id;

			track.keys.resize(key_count);

			for (int j = 0; j < key_count; j++) {
				track.keys.write[j].time = time_keys[j];
				state.animation_length = MAX(state.animation_length, time_keys[j]);
			}

			// now read actual values

			int stride = 1;

			if (source_strides.has(output_id)) {
				stride = source_strides[output_id];
			}
			int output_len = stride / names.size();

			ERR_CONTINUE(output_len == 0);
			ERR_CONTINUE(!float_sources.has(output_id));

			Vector<float>& output = float_sources[output_id];

			ERR_CONTINUE_MSG(
				(output.size() / stride) != key_count, "Wrong number of keys in output.");

			for (int j = 0; j < key_count; j++) {
				track.keys.write[j].data.resize(output_len);
				for (int k = 0; k < output_len; k++) {
					track.keys.write[j].data.write[k] =
						output[l + j * stride + k]; // super weird but should work:
				}
			}

			if (sampler.has("INTERPOLATION")) {
				String interp_id = _uri_to_id(sampler["INTERPOLATION"]);
				ERR_CONTINUE(!string_sources.has(interp_id));
				Vector<String>& interps = string_sources[interp_id];
				ERR_CONTINUE(interps.size() != key_count);

				for (int j = 0; j < key_count; j++) {
					if (interps[j] == "BEZIER") {
						track.keys.write[j].interp_type = AnimationTrack::INTERP_BEZIER;
					}
					else {
						track.keys.write[j].interp_type = AnimationTrack::INTERP_LINEAR;
					}
				}
			}

			if (sampler.has("IN_TANGENT") && sampler.has("OUT_TANGENT")) {
				// bezier control points..
				String intangent_id = _uri_to_id(sampler["IN_TANGENT"]);
				ERR_CONTINUE(!float_sources.has(intangent_id));
				Vector<float>& intangents = float_sources[intangent_id];

				ERR_CONTINUE(intangents.size() != key_count * 2 * names.size());

				String outangent_id = _uri_to_id(sampler["OUT_TANGENT"]);
				ERR_CONTINUE(!float_sources.has(outangent_id));
				Vector<float>& outangents = float_sources[outangent_id];
				ERR_CONTINUE(outangents.size() != key_count * 2 * names.size());

				for (int j = 0; j < key_count; j++) {
					track.keys.write[j].in_tangent =
						Vector2(intangents[j * 2 * names.size() + 0 + l * 2],
							intangents[j * 2 * names.size() + 1 + l * 2]);
					track.keys.write[j].out_tangent =
						Vector2(outangents[j * 2 * names.size() + 0 + l * 2],
							outangents[j * 2 * names.size() + 1 + l * 2]);
				}
			}

			if (target.contains_char('/')) { // transform component
				track.target = target.get_slicec('/', 0);
				track.param = target.get_slicec('/', 1);
				if (track.param.contains_char('.')) {
					track.component = track.param.get_slicec('.', 1).to_upper();
				}
				track.param = track.param.get_slicec('.', 0);
				if (names.size() > 1 && track.component.is_empty()) {
					// this is a guess because the collada spec is ambiguous here...
					// i suppose if you have many names (outputs) you can't use a component and i
					// should abide to that.
					track.component = name;
				}
			}
			else {
				track.target = target;
			}

			state.animation_tracks.push_back(track);

			if (!state.referenced_tracks.has(target)) {
				state.referenced_tracks[target] = Vector<int>();
			}

			state.referenced_tracks[target].push_back(state.animation_tracks.size() - 1);

			if (!id.is_empty()) {
				if (!state.by_id_tracks.has(id)) {
					state.by_id_tracks[id] = Vector<int>();
				}

				state.by_id_tracks[id].push_back(state.animation_tracks.size() - 1);
			}

			COLLADA_PRINT("loaded animation with " + itos(key_count) + " keys");
		}
	}
}

void Collada::_parse_animation_clip(XMLParser& p_parser)
{
	if (!(state.import_flags & IMPORT_FLAG_ANIMATION)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}

		return;
	}

	AnimationClip clip;

	if (p_parser.has_attribute("name")) {
		clip.name = p_parser.get_named_attribute_value("name");
	}
	else if (p_parser.has_attribute("id")) {
		clip.name = p_parser.get_named_attribute_value("id");
	}
	if (p_parser.has_attribute("start")) {
		clip.begin = p_parser.get_named_attribute_value("start").to_float();
	}
	if (p_parser.has_attribute("end")) {
		clip.end = p_parser.get_named_attribute_value("end").to_float();
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();
			if (name == "instance_animation") {
				String url = _uri_to_id(p_parser.get_named_attribute_value("url"));
				clip.tracks.push_back(url);
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "animation_clip") {
			break; // end of <asset>
		}
	}

	state.animation_clips.push_back(clip);
}

void Collada::_parse_scene(XMLParser& p_parser)
{
	if (p_parser.is_empty()) {
		return;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();

			if (name == "instance_visual_scene") {
				state.root_visual_scene = _uri_to_id(p_parser.get_named_attribute_value("url"));
			}
			else if (name == "instance_physics_scene") {
				state.root_physics_scene = _uri_to_id(p_parser.get_named_attribute_value("url"));
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "scene") {
			break; // end of <asset>
		}
	}
}

void Collada::_parse_library(XMLParser& p_parser)
{
	if (p_parser.is_empty()) {
		return;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();
			COLLADA_PRINT("library name is: " + name);
			if (name == "image") {
				_parse_image(p_parser);
			}
			else if (name == "material") {
				_parse_material(p_parser);
			}
			else if (name == "effect") {
				_parse_effect(p_parser);
			}
			else if (name == "camera") {
				_parse_camera(p_parser);
			}
			else if (name == "light") {
				_parse_light(p_parser);
			}
			else if (name == "geometry") {
				String id = p_parser.get_named_attribute_value("id");
				String name2 = p_parser.get_named_attribute_value_safe("name");
				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "mesh") {
							state.mesh_name_map[id] = (!name2.is_empty()) ? name2 : id;
							_parse_mesh_geometry(p_parser, id, name2);
						}
						else if (p_parser.get_node_name() == "spline") {
							state.mesh_name_map[id] = (!name2.is_empty()) ? name2 : id;
							_parse_curve_geometry(p_parser, id, name2);
						}
						else if (!p_parser.is_empty()) {
							p_parser.skip_section();
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == "geometry") {
						break;
					}
				}

			}
			else if (name == "controller") {
				_parse_controller(p_parser);
			}
			else if (name == "animation") {
				_parse_animation(p_parser);
			}
			else if (name == "animation_clip") {
				_parse_animation_clip(p_parser);
			}
			else if (name == "visual_scene") {
				COLLADA_PRINT("visual scene");
				_parse_visual_scene(p_parser);
			}
			else if (!p_parser.is_empty()) {
				p_parser.skip_section();
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name().begins_with("library_")) {
			break; // end of <asset>
		}
	}
}

void Collada::_joint_set_owner(Collada::Node* p_node, NodeSkeleton* p_owner)
{
	if (p_node->type == Node::TYPE_JOINT) {
		NodeJoint* nj = static_cast<NodeJoint*>(p_node);
		nj->owner = p_owner;

		for (int i = 0; i < nj->children.size(); i++) {
			_joint_set_owner(nj->children.write[i], p_owner);
		}
	}
}

void Collada::_create_skeletons(Collada::Node** p_node, NodeSkeleton* p_skeleton)
{
	Node* node = *p_node;

	if (node->type == Node::TYPE_JOINT) {
		if (!p_skeleton) {
			// ohohohoohoo it's a joint node, time to work!
			NodeSkeleton* sk = memnew(NodeSkeleton);
			*p_node = sk;
			sk->children.push_back(node);
			sk->parent = node->parent;
			node->parent = sk;
			p_skeleton = sk;
		}

		NodeJoint* nj = static_cast<NodeJoint*>(node);
		nj->owner = p_skeleton;
	}
	else {
		p_skeleton = nullptr;
	}

	for (int i = 0; i < node->children.size(); i++) {
		_create_skeletons(&node->children.write[i], p_skeleton);
	}
}

bool Collada::_remove_node(Node* p_parent, Node* p_node)
{
	for (int i = 0; i < p_parent->children.size(); i++) {
		if (p_parent->children[i] == p_node) {
			p_parent->children.remove_at(i);
			return true;
		}
		if (_remove_node(p_parent->children[i], p_node)) {
			return true;
		}
	}

	return false;
}

void Collada::_remove_node(VisualScene* p_vscene, Node* p_node)
{
	for (int i = 0; i < p_vscene->root_nodes.size(); i++) {
		if (p_vscene->root_nodes[i] == p_node) {
			p_vscene->root_nodes.remove_at(i);
			return;
		}
		if (_remove_node(p_vscene->root_nodes[i], p_node)) {
			return;
		}
	}

	ERR_PRINT("Not found node to remove?");
}

void Collada::_merge_skeletons(VisualScene* p_vscene, Node* p_node)
{
	if (p_node->type == Node::TYPE_GEOMETRY) {
		NodeGeometry* gnode = static_cast<NodeGeometry*>(p_node);
		if (gnode->controller) {
			// recount skeletons used
			HashSet<NodeSkeleton*> skeletons;

			for (int i = 0; i < gnode->skeletons.size(); i++) {
				String nodeid = gnode->skeletons[i];

				ERR_CONTINUE(!state.scene_map.has(nodeid)); // weird, it should have it...

				NodeJoint* nj = dynamic_cast<NodeJoint*>(state.scene_map[nodeid]);
				ERR_CONTINUE(!nj);		  // broken collada
				ERR_CONTINUE(!nj->owner); // weird, node should have a skeleton owner

				skeletons.insert(nj->owner);
			}

			if (skeletons.size() > 1) {
				// do the merger!!
				HashSet<NodeSkeleton*>::Iterator E = skeletons.begin();
				NodeSkeleton* base = *E;

				for (++E; E; ++E) {
					NodeSkeleton* merged = *E;
					_remove_node(p_vscene, merged);
					for (int i = 0; i < merged->children.size(); i++) {
						_joint_set_owner(merged->children[i], base);
						base->children.push_back(merged->children[i]);
						merged->children[i]->parent = base;
					}

					merged->children.clear(); // take children from it
					memdelete(merged);
				}
			}
		}
	}

	for (int i = 0; i < p_node->children.size(); i++) {
		_merge_skeletons(p_vscene, p_node->children[i]);
	}
}

void Collada::_merge_skeletons2(VisualScene* p_vscene)
{
	for (KeyValue<String, SkinControllerData>& E : state.skin_controller_data_map) {
		SkinControllerData& cd = E.value;

		NodeSkeleton* skeleton = nullptr;

		for (const KeyValue<String, Transform3D>& F : cd.bone_rest_map) {
			String name;

			if (!state.sid_to_node_map.has(F.key)) {
				continue;
			}

			name = state.sid_to_node_map[F.key];

			ERR_CONTINUE(!state.scene_map.has(name));

			Node* node = state.scene_map[name];
			ERR_CONTINUE(node->type != Node::TYPE_JOINT);

			NodeSkeleton* sk = nullptr;

			while (node && !sk) {
				if (node->type == Node::TYPE_SKELETON) {
					sk = static_cast<NodeSkeleton*>(node);
				}
				node = node->parent;
			}

			ERR_CONTINUE(!sk);

			if (!skeleton) {
				skeleton = sk;
				continue;
			}

			if (skeleton != sk) {
				// whoa.. wtf, merge.
				_remove_node(p_vscene, sk);
				for (int i = 0; i < sk->children.size(); i++) {
					_joint_set_owner(sk->children[i], skeleton);
					skeleton->children.push_back(sk->children[i]);
					sk->children[i]->parent = skeleton;
				}

				sk->children.clear(); // take children from it
				memdelete(sk);
			}
		}
	}
}

bool Collada::_optimize_skeletons(VisualScene* p_vscene, Node* p_node)
{
	Node* node = p_node;

	if (node->type == Node::TYPE_SKELETON && node->parent &&
		node->parent->type == Node::TYPE_NODE && node->parent->children.size() == 1) {
		// replace parent by this...
		Node* parent = node->parent;

		// i wonder if this is alright.. i think it is since created skeleton (first joint) is
		// already animated by bone..
		node->id = parent->id;
		node->name = parent->name;
		node->xform_list = parent->xform_list;
		node->default_transform = parent->default_transform;

		state.scene_map[node->id] = node;
		node->parent = parent->parent;

		if (parent->parent) {
			Node* gp = parent->parent;
			bool found = false;
			for (int i = 0; i < gp->children.size(); i++) {
				if (gp->children[i] == parent) {
					gp->children.write[i] = node;
					found = true;
					break;
				}
			}
			if (!found) {
				ERR_PRINT("BUG");
			}
		}
		else {
			bool found = false;

			for (int i = 0; i < p_vscene->root_nodes.size(); i++) {
				if (p_vscene->root_nodes[i] == parent) {
					p_vscene->root_nodes.write[i] = node;
					found = true;
					break;
				}
			}
			if (!found) {
				ERR_PRINT("BUG");
			}
		}

		parent->children.clear();
		memdelete(parent);
		return true;
	}

	for (int i = 0; i < node->children.size(); i++) {
		if (_optimize_skeletons(p_vscene, node->children[i])) {
			return false; // stop processing, go up
		}
	}

	return false;
}

bool Collada::_move_geometry_to_skeletons(VisualScene* p_vscene, Node* p_node, List<Node*>* p_mgeom)
{
	// Bind Shape Matrix scales the bones and makes them gigantic, so the matrix then shrinks the
	// model? Solution: apply the Bind Shape Matrix to the VERTICES, and if the object comes scaled,
	// it seems to be left alone!

	if (p_node->type == Node::TYPE_GEOMETRY) {
		NodeGeometry* ng = static_cast<NodeGeometry*>(p_node);
		if (ng->ignore_anim) {
			return false; // already made child of skeleton and processeg
		}

		if (ng->controller && ng->skeletons.size()) {
			String nodeid = ng->skeletons[0];

			ERR_FAIL_COND_V(!state.scene_map.has(nodeid), false); // weird, it should have it...
			NodeJoint* nj = dynamic_cast<NodeJoint*>(state.scene_map[nodeid]);
			ERR_FAIL_NULL_V(nj, false);
			ERR_FAIL_NULL_V(nj->owner, false); // Weird, node should have a skeleton owner.

			NodeSkeleton* sk = nj->owner;

			Node* p = sk->parent;
			bool node_is_parent_of_skeleton = false;

			while (p) {
				if (p == p_node) {
					node_is_parent_of_skeleton = true;
					break;
				}
				p = p->parent; // try again
			}

			ERR_FAIL_COND_V(node_is_parent_of_skeleton, false);

			// this should be correct
			ERR_FAIL_COND_V(!state.skin_controller_data_map.has(ng->source), false);
			SkinControllerData& skin = state.skin_controller_data_map[ng->source];
			Transform3D skel_inv = sk->get_global_transform().affine_inverse();
			p_node->default_transform =
				skel_inv *
				(skin.bind_shape /* p_node->get_global_transform()*/); // i honestly have no idea
																	   // what to do with a previous
																	   // model xform.. most
																	   // exporters ignore it

			// make rests relative to the skeleton (they seem to be always relative to world)
			for (KeyValue<String, Transform3D>& E : skin.bone_rest_map) {
				E.value = skel_inv * E.value; // make the bone rest local to the skeleton
				state.bone_rest_map[E.key] =
					E.value; // make it remember where the bone is globally, now that it's relative
			}

			// but most exporters seem to work only if i do this..
			// p_node->default_transform = p_node->get_global_transform();

			// p_node->default_transform=Transform3D(); //this seems to be correct, because bind
			// shape makes the object local to the skeleton
			p_node->ignore_anim =
				true; // collada may animate this later, if it does, then this is not supported
					  // (redo your original asset and don't animate the base mesh)
			p_node->parent = sk;
			// sk->children.push_back(0,p_node); //avoid INFINITE loop
			p_mgeom->push_back(p_node);
			return true;
		}
	}

	for (int i = 0; i < p_node->children.size(); i++) {
		if (_move_geometry_to_skeletons(p_vscene, p_node->children[i], p_mgeom)) {
			p_node->children.remove_at(i);
			i--;
		}
	}

	return false;
}

void Collada::_find_morph_nodes(VisualScene* p_vscene, Node* p_node)
{
	if (p_node->type == Node::TYPE_GEOMETRY) {
		NodeGeometry* nj = static_cast<NodeGeometry*>(p_node);

		if (nj->controller) {
			String base = nj->source;

			while (!base.is_empty() && !state.mesh_data_map.has(base)) {
				if (state.skin_controller_data_map.has(base)) {
					SkinControllerData& sk = state.skin_controller_data_map[base];
					base = sk.base;
				}
				else if (state.morph_controller_data_map.has(base)) {
					state.morph_ownership_map[base] = nj->id;
					break;
				}
				else {
					ERR_FAIL_MSG("Invalid scene.");
				}
			}
		}
	}

	for (int i = 0; i < p_node->children.size(); i++) {
		_find_morph_nodes(p_vscene, p_node->children[i]);
	}
}

void Collada::_optimize()
{
	for (KeyValue<String, VisualScene>& E : state.visual_scene_map) {
		VisualScene& vs = E.value;
		for (int i = 0; i < vs.root_nodes.size(); i++) {
			_create_skeletons(&vs.root_nodes.write[i]);
		}


	for (int i = 0; i < vs.root_nodes.size(); i++) {
			_merge_skeletons(&vs, vs.root_nodes[i]);
		}

		_merge_skeletons2(&vs);

		for (int i = 0; i < vs.root_nodes.size(); i++) {
			_optimize_skeletons(&vs, vs.root_nodes[i]);
		}

		for (int i = 0; i < vs.root_nodes.size(); i++) {
			List<Node*> mgeom;
			if (_move_geometry_to_skeletons(&vs, vs.root_nodes[i], &mgeom)) {
				vs.root_nodes.remove_at(i);
				i--;
			}

			while (!mgeom.is_empty()) {
				Node* n = mgeom.front()->get();
				n->parent->children.push_back(n);
				mgeom.pop_front();
			}
		}

		for (int i = 0; i < vs.root_nodes.size(); i++) {
			_find_morph_nodes(&vs, vs.root_nodes[i]);
		}
	}
}

int Collada::get_uv_channel(const String& p_name)
{
	if (!channel_map.has(p_name)) {
		ERR_FAIL_COND_V(channel_map.size() == 2, 0);

		channel_map[p_name] = channel_map.size();
	}

	return channel_map[p_name];
}

Error Collada::load(const String& p_path, int p_flags)
{
	Ref<XMLParser> parserr = memnew(XMLParser);
	XMLParser& parser = *parserr.ptr();
	Error err = parser.open(p_path);
	ERR_FAIL_COND_V_MSG(err, err, "Cannot open Collada file '" + p_path + "'.");

	state.local_path = ProjectSettings::localize_path(p_path);
	state.import_flags = p_flags;
	/* Skip headers */
	while ((err = parser.read()) == OK) {
		if (parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			if (parser.get_node_name() == "COLLADA") {
				break;
			}
			else if (!parser.is_empty()) {
				parser.skip_section(); // unknown section, likely headers
			}
		}
	}

	ERR_FAIL_COND_V_MSG(err != OK, ERR_FILE_CORRUPT, "Corrupted Collada file '" + p_path + "'.");

	/* Start loading Collada */

	{
		// version
		String version = parser.get_named_attribute_value("version");
		state.version.major = version.get_slicec('.', 0).to_int();
		state.version.minor = version.get_slicec('.', 1).to_int();
		state.version.rev = version.get_slicec('.', 2).to_int();
		COLLADA_PRINT("Collada VERSION: " + version);
	}

	while ((err = parser.read()) == OK) {
		/* Read all the main sections.. */

		if (parser.get_node_type() != XMLParser::NODE_ELEMENT) {
			continue; // no idea what this may be, but skipping anyway
		}

		String section = parser.get_node_name();

		COLLADA_PRINT("section: " + section);

		if (section == "asset") {
			_parse_asset(parser);

		}
		else if (section.begins_with("library_")) {
			_parse_library(parser);
		}
		else if (section == "scene") {
			_parse_scene(parser);
		}
		else if (!parser.is_empty()) {
			parser.skip_section(); // unknown section, likely headers
		}
	}

	_optimize();
	return OK;
}

void Collada::_parse_effect_material(XMLParser& p_parser, Collada::Effect& r_effect, String& r_id) {}

#ifdef COLLADA_PRINT
#undef COLLADA_PRINT
#endif
#ifdef COLLADA_IMPORT_SCALE_SCENE
#undef COLLADA_IMPORT_SCALE_SCENE
#endif

// ============================================================================
// Merged from editor/import/3d/editor_import_collada.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/templates/rb_set.h"

// --- Inlined from editor/import/3d/editor_import_collada.h ---
#include "editor/import/3d/resource_importer_scene.h"

class EditorSceneFormatImporterCollada : public EditorSceneFormatImporter
{
public:
	virtual void get_extensions(List<String>* r_extensions) const override;
};



// --- End editor/import/3d/editor_import_collada.h ---
#include "scene/3d/camera_3d.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/path_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/animation.h"
#include "scene/resources/surface_tool.h"

struct ColladaImport
{
	Collada collada;
	Node3D* scene = nullptr;

	Vector<Ref<Animation>> animations;

	struct NodeMap
	{
		// String path;
		Node3D* node = nullptr;
		int bone = -1;
		List<int> anim_tracks;
	};

	bool found_ambient = false;
	Color ambient;
	bool found_directional = false;
	bool force_make_tangents = false;
	bool apply_mesh_xform_to_vertices = true;
	bool use_mesh_builtin_materials = false;
	float bake_fps = 30;

	HashMap<String, NodeMap> node_map;	   // map from collada node to engine node
	HashMap<String, String> node_name_map; // map from collada node to engine node
	HashMap<String, Ref<ImporterMesh>> mesh_cache;
	HashMap<String, Ref<Curve3D>> curve_cache;
	HashMap<String, Ref<Material>> material_cache;
	HashMap<Collada::Node*, Skeleton3D*> skeleton_map;

	HashMap<Skeleton3D*, HashMap<String, int>> skeleton_bone_map;

	HashSet<String> valid_animated_nodes;
	Vector<int> valid_animated_properties;
	HashMap<String, bool> bones_with_animation;

	HashSet<String> mesh_unique_names;
	HashSet<String> material_unique_names;

	Error _populate_skeleton(
		Skeleton3D* p_skeleton, Collada::Node* p_node, int& r_bone, int p_parent);
	Error _create_scene_skeletons(Collada::Node* p_node);
	Error _create_scene(Collada::Node* p_node, Node3D* p_parent);
	Error _create_resources(Collada::Node* p_node, bool p_use_compression);
	Error _create_material(const String& p_target);
	Error _create_mesh_surfaces(bool p_optimize, Ref<ImporterMesh>& p_mesh,
		const HashMap<String, Collada::NodeGeometry::Material>& p_material_map,
		const Collada::MeshData& meshdata, const Transform3D& p_local_xform,
		const Vector<int>& bone_remap, const Collada::SkinControllerData* p_skin_controller,
		const Collada::MorphControllerData* p_morph_data,
		const Vector<Ref<ImporterMesh>>& p_morph_meshes = Vector<Ref<ImporterMesh>>(),
		bool p_use_compression = false, bool p_use_mesh_material = false);
	Error load(const String& p_path, int p_flags, bool p_force_make_tangents = false,
		bool p_use_compression = false);
	void _fix_param_animation_tracks();
	void create_animation(int p_clip, bool p_import_value_tracks);
	void create_animations(bool p_import_value_tracks);

	HashSet<String> tracks_in_clips;
	Vector<String> missing_textures;

	void _pre_process_lights(Collada::Node* p_node);
};

Error ColladaImport::_populate_skeleton(
	Skeleton3D* p_skeleton, Collada::Node* p_node, int& r_bone, int p_parent)
{
	if (p_node->type != Collada::Node::TYPE_JOINT) {
		return OK;
	}

	Collada::NodeJoint* joint = static_cast<Collada::NodeJoint*>(p_node);

	p_skeleton->add_bone(p_node->name);
	if (p_parent >= 0) {
		p_skeleton->set_bone_parent(r_bone, p_parent);
	}

	NodeMap nm;
	nm.node = p_skeleton;
	nm.bone = r_bone;
	node_map[p_node->id] = nm;
	node_name_map[p_node->name] = p_node->id;

	skeleton_bone_map[p_skeleton][joint->sid] = r_bone;

	{
		Transform3D xform = joint->compute_transform(collada);
		xform = collada.fix_transform(xform) * joint->post_transform;

		p_skeleton->set_bone_pose_position(r_bone, xform.origin);
		p_skeleton->set_bone_pose_rotation(r_bone, xform.basis.get_rotation_quaternion());
		p_skeleton->set_bone_pose_scale(r_bone, xform.basis.get_scale());
	}

	if (collada.state.bone_rest_map.has(joint->sid)) {
		p_skeleton->set_bone_rest(
			r_bone, collada.fix_transform(collada.state.bone_rest_map[joint->sid]));
		// should map this bone to something for animation?
	}
	else {
		WARN_PRINT("Collada: Joint has no rest.");
	}

	int id = r_bone++;
	for (int i = 0; i < p_node->children.size(); i++) {
		Error err = _populate_skeleton(p_skeleton, p_node->children[i], r_bone, id);
		if (err) {
			return err;
		}
	}

	return OK;
}

void ColladaImport::_pre_process_lights(Collada::Node* p_node)
{
	if (p_node->type == Collada::Node::TYPE_LIGHT) {
		Collada::NodeLight* light = static_cast<Collada::NodeLight*>(p_node);
		if (collada.state.light_data_map.has(light->light)) {
			Collada::LightData& ld = collada.state.light_data_map[light->light];
			if (ld.mode == Collada::LightData::MODE_AMBIENT) {
				found_ambient = true;
				ambient = ld.color;
			}
			if (ld.mode == Collada::LightData::MODE_DIRECTIONAL) {
				found_directional = true;
			}
		}
	}

	for (int i = 0; i < p_node->children.size(); i++) {
		_pre_process_lights(p_node->children[i]);
	}
}

Error ColladaImport::_create_scene_skeletons(Collada::Node* p_node)
{
	if (p_node->type == Collada::Node::TYPE_SKELETON) {
		Skeleton3D* sk = memnew(Skeleton3D);
		int bone = 0;
		for (int i = 0; i < p_node->children.size(); i++) {
			_populate_skeleton(sk, p_node->children[i], bone, -1);
		}
		sk->localize_rests(); // after creating skeleton, rests must be localized...!
		skeleton_map[p_node] = sk;
	}

	for (int i = 0; i < p_node->children.size(); i++) {
		Error err = _create_scene_skeletons(p_node->children[i]);
		if (err) {
			return err;
		}
	}
	return OK;
}

Error ColladaImport::_create_material(const String& p_target)
{
	ERR_FAIL_COND_V(material_cache.has(p_target), ERR_ALREADY_EXISTS);
	ERR_FAIL_COND_V(!collada.state.material_map.has(p_target), ERR_INVALID_PARAMETER);
	Collada::Material& src_mat = collada.state.material_map[p_target];
	ERR_FAIL_COND_V(!collada.state.effect_map.has(src_mat.instance_effect), ERR_INVALID_PARAMETER);
	Collada::Effect& effect = collada.state.effect_map[src_mat.instance_effect];

	Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);

	String base_name;
	if (!src_mat.name.is_empty()) {
		base_name = src_mat.name;
	}
	else if (!effect.name.is_empty()) {
		base_name = effect.name;
	}
	else {
		base_name = "Material";
	}

	String name = base_name;
	int counter = 2;
	while (material_unique_names.has(name)) {
		name = base_name + itos(counter++);
	}

	material_unique_names.insert(name);

	material->set_name(name);

	// DIFFUSE

	if (!effect.diffuse.texture.is_empty()) {
		String texfile = effect.get_texture_path(effect.diffuse.texture, collada);
		if (!texfile.is_empty()) {
			if (texfile.begins_with("/")) {
				texfile = texfile.replace_first("/", "res://");
			}
			Ref<Texture2D> texture = ResourceLoader::load(texfile, "Texture2D");
			if (texture.is_valid()) {
				material->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, texture);
				// material->set_parameter(StandardMaterial3D::PARAM_DIFFUSE,Color(1,1,1,1));
			}
			else {
				missing_textures.push_back(texfile.get_file());
			}
		}
	}

	// SPECULAR

	if (!effect.specular.texture.is_empty()) {
		String texfile = effect.get_texture_path(effect.specular.texture, collada);
		if (!texfile.is_empty()) {
			if (texfile.begins_with("/")) {
				texfile = texfile.replace_first("/", "res://");
			}

			Ref<Texture2D> texture = ResourceLoader::load(texfile, "Texture2D");
			if (texture.is_valid()) {
				material->set_texture(StandardMaterial3D::TEXTURE_METALLIC, texture);
				material->set_specular(1.0);

				// material->set_texture(StandardMaterial3D::PARAM_SPECULAR,texture);
				// material->set_parameter(StandardMaterial3D::PARAM_SPECULAR,Color(1,1,1,1));
			}
			else {
				missing_textures.push_back(texfile.get_file());
			}
		}

	}
	else {
		material->set_metallic(effect.specular.color.get_v());
	}

	// EMISSION

	if (!effect.emission.texture.is_empty()) {
		String texfile = effect.get_texture_path(effect.emission.texture, collada);
		if (!texfile.is_empty()) {
			if (texfile.begins_with("/")) {
				texfile = texfile.replace_first("/", "res://");
			}

			Ref<Texture2D> texture = ResourceLoader::load(texfile, "Texture2D");
			if (texture.is_valid()) {
				material->set_feature(StandardMaterial3D::FEATURE_EMISSION, true);
				material->set_texture(StandardMaterial3D::TEXTURE_EMISSION, texture);
				material->set_emission(Color(1, 1, 1, 1));

				// material->set_parameter(StandardMaterial3D::PARAM_EMISSION,Color(1,1,1,1));
			}
			else {
				missing_textures.push_back(texfile.get_file());
			}
		}
	}
	else {
		if (effect.emission.color != Color()) {
			material->set_feature(StandardMaterial3D::FEATURE_EMISSION, true);
			material->set_emission(effect.emission.color);
		}
	}

	// NORMAL

	if (!effect.bump.texture.is_empty()) {
		String texfile = effect.get_texture_path(effect.bump.texture, collada);
		if (!texfile.is_empty()) {
			if (texfile.begins_with("/")) {
				texfile = texfile.replace_first("/", "res://");
			}

			Ref<Texture2D> texture = ResourceLoader::load(texfile, "Texture2D");
			if (texture.is_valid()) {
				material->set_feature(StandardMaterial3D::FEATURE_NORMAL_MAPPING, true);
				material->set_texture(StandardMaterial3D::TEXTURE_NORMAL, texture);
				// material->set_emission(Color(1,1,1,1));

				// material->set_texture(StandardMaterial3D::PARAM_NORMAL,texture);
			}
			else {
				// missing_textures.push_back(texfile.get_file());
			}
		}
	}

	float roughness = (effect.shininess - 1.0) / 510;
	material->set_roughness(roughness);

	if (effect.double_sided) {
		material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	}
	if (effect.unshaded) {
		material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	}

	material_cache[p_target] = material;
	return OK;
}

Error ColladaImport::load(
	const String& p_path, int p_flags, bool p_force_make_tangents, bool p_use_compression)
{
	Error err = collada.load(p_path, p_flags);
	ERR_FAIL_COND_V_MSG(err, err, "Cannot load file '" + p_path + "'.");

	force_make_tangents = p_force_make_tangents;
	ERR_FAIL_COND_V(
		!collada.state.visual_scene_map.has(collada.state.root_visual_scene), ERR_INVALID_DATA);
	Collada::VisualScene& vs = collada.state.visual_scene_map[collada.state.root_visual_scene];

	scene = memnew(Node3D); // root

	// determine what's going on with the lights
	for (int i = 0; i < vs.root_nodes.size(); i++) {
		_pre_process_lights(vs.root_nodes[i]);
	}
	// import scene

	for (int i = 0; i < vs.root_nodes.size(); i++) {
		Error err2 = _create_scene_skeletons(vs.root_nodes[i]);
		if (err2 != OK) {
			memdelete(scene);
			ERR_FAIL_COND_V(err2, err2);
		}
	}

	for (int i = 0; i < vs.root_nodes.size(); i++) {
		Error err2 = _create_scene(vs.root_nodes[i], scene);
		if (err2 != OK) {
			memdelete(scene);
			ERR_FAIL_COND_V(err2, err2);
		}

		Error err3 = _create_resources(vs.root_nodes[i], p_use_compression);
		if (err3 != OK) {
			memdelete(scene);
			ERR_FAIL_COND_V(err3, err3);
		}
	}

	// optatively, set unit scale in the root
	scene->set_transform(collada.get_root_transform());

	return OK;
}

void ColladaImport::_fix_param_animation_tracks()
{
	for (KeyValue<String, Collada::Node*>& E : collada.state.scene_map) {
		Collada::Node* n = E.value;
		switch (n->type) {
		case Collada::Node::TYPE_NODE: {
			// ? do nothing
		} break;
		case Collada::Node::TYPE_JOINT: {
		} break;
		case Collada::Node::TYPE_SKELETON: {
		} break;
		case Collada::Node::TYPE_LIGHT: {
		} break;
		case Collada::Node::TYPE_CAMERA: {
		} break;
		case Collada::Node::TYPE_GEOMETRY: {
			Collada::NodeGeometry* ng = static_cast<Collada::NodeGeometry*>(n);
			// test source(s)
			String source = ng->source;

			while (!source.is_empty()) {
				if (collada.state.skin_controller_data_map.has(source)) {
					const Collada::SkinControllerData& skin =
						collada.state.skin_controller_data_map[source];

					// nothing to animate here i think

					source = skin.base;
				}
				else if (collada.state.morph_controller_data_map.has(source)) {
					const Collada::MorphControllerData& morph =
						collada.state.morph_controller_data_map[source];

					if (morph.targets.has("MORPH_WEIGHT") && morph.targets.has("MORPH_TARGET")) {
						String weights = morph.targets["MORPH_WEIGHT"];
						String targets = morph.targets["MORPH_TARGET"];
						// fails here

						if (morph.sources.has(targets) && morph.sources.has(weights)) {
							const Collada::MorphControllerData::Source& weight_src =
								morph.sources[weights];
							const Collada::MorphControllerData::Source& target_src =
								morph.sources[targets];

							ERR_FAIL_COND(weight_src.array.size() != target_src.sarray.size());

							for (int i = 0; i < weight_src.array.size(); i++) {
								String track_name = weights + "(" + itos(i) + ")";
								String mesh_name = target_src.sarray[i];
								if (collada.state.mesh_name_map.has(mesh_name) &&
									collada.state.referenced_tracks.has(track_name)) {
									const Vector<int>& rt =
										collada.state.referenced_tracks[track_name];

									for (int rti = 0; rti < rt.size(); rti++) {
										Collada::AnimationTrack* at =
											&collada.state.animation_tracks.write[rt[rti]];

										at->target = E.key;
										at->param =
											"morph/" + collada.state.mesh_name_map[mesh_name];
										at->property = true;
										// at->param
									}
								}
							}
						}
					}
					source = morph.mesh;
				}
				else {
					source = ""; // for now nothing else supported
				}
			}

		} break;
		}
	}
}

void ColladaImport::create_animations(bool p_import_value_tracks)
{
	_fix_param_animation_tracks();
	for (int i = 0; i < collada.state.animation_clips.size(); i++) {
		for (int j = 0; j < collada.state.animation_clips[i].tracks.size(); j++) {
			tracks_in_clips.insert(collada.state.animation_clips[i].tracks[j]);
		}
	}

	for (int i = 0; i < collada.state.animation_tracks.size(); i++) {
		const Collada::AnimationTrack& at = collada.state.animation_tracks[i];

		String node;

		if (!node_map.has(at.target)) {
			if (node_name_map.has(at.target)) {
				node = node_name_map[at.target];
			}
			else {
				WARN_PRINT("Collada: Couldn't find node: " + at.target);
				continue;
			}
		}
		else {
			node = at.target;
		}

		if (at.property) {
			valid_animated_properties.push_back(i);

		}
		else {
			node_map[node].anim_tracks.push_back(i);
			valid_animated_nodes.insert(node);
		}
	}

	create_animation(-1, p_import_value_tracks);
	for (int i = 0; i < collada.state.animation_clips.size(); i++) {
		create_animation(i, p_import_value_tracks);
	}
}

void EditorSceneFormatImporterCollada::get_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("dae");
}

void ColladaImport::create_animation(int p_clip, bool p_bake) {}

Error ColladaImport::_create_scene(Collada::Node* p_node, Node3D* p_parent) { return OK; }

Error ColladaImport::_create_resources(Collada::Node* p_node, bool p_is_root) { return OK; }



// ============================================================================
// Merged from editor/import/3d/resource_importer_obj.cpp
// ============================================================================

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
// --- Inlined from editor/import/3d/resource_importer_obj.h ---
#include "editor/import/3d/resource_importer_scene.h"

class EditorOBJImporter : public EditorSceneFormatImporter
{
public:
	virtual void get_extensions(List<String>* r_extensions) const override;
};

class ResourceImporterOBJ : public ResourceImporter
{
public:
	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;
	virtual int get_format_version() const override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;
};



// --- End editor/import/3d/resource_importer_obj.h ---
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/mesh.h"
#include "scene/resources/surface_tool.h"

static Error _parse_material_library(const String& p_path,
	HashMap<String, Ref<StandardMaterial3D>>& material_map, List<String>* r_missing_deps)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN,
		vformat("Couldn't open MTL file '%s', it may not exist or not be readable.", p_path));

	Ref<StandardMaterial3D> current;
	String current_name;
	String base_path = p_path.get_base_dir();
	while (true) {
		String l = f->get_line().strip_edges();

		if (l.begins_with("newmtl ")) {
			// Start of a new material.
			current_name = l.replace("newmtl", "").strip_edges();
			current.instantiate();
			current->set_name(current_name);
			material_map[current_name] = current;
		}
		else if (l.begins_with("Ka ")) {
			// Ambient color.
			WARN_PRINT("OBJ: Ambient light for material '" + current_name + "' is ignored in PBR");

		}
		else if (l.begins_with("Kd ")) {
			// Diffuse color.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);
			Vector<String> v = l.split(" ", false);
			ERR_FAIL_COND_V(v.size() < 4, ERR_INVALID_DATA);
			Color c = current->get_albedo();
			c.r = v[1].to_float();
			c.g = v[2].to_float();
			c.b = v[3].to_float();
		}
		else if (l.begins_with("Ks ")) {
			// Specular color.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);
			Vector<String> v = l.split(" ", false);
			ERR_FAIL_COND_V(v.size() < 4, ERR_INVALID_DATA);
			float r = v[1].to_float();
			float g = v[2].to_float();
			float b = v[3].to_float();
			float metalness = MAX(r, MAX(g, b));
			current->set_metallic(metalness);
		}
		else if (l.begins_with("Ns ")) {
			// Specular exponent.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);
			Vector<String> v = l.split(" ", false);
			ERR_FAIL_COND_V(v.size() != 2, ERR_INVALID_DATA);
			float s = v[1].to_float();
			current->set_metallic((1000.0 - s) / 1000.0);
		}
		else if (l.begins_with("d ")) {
			// Dissolve (1.0 is fully opaque, 0.0 is completely transparent).
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);
			Vector<String> v = l.split(" ", false);
			ERR_FAIL_COND_V(v.size() != 2, ERR_INVALID_DATA);
			float d = v[1].to_float();
			Color c = current->get_albedo();
			c.a = d;
			if (c.a < 0.99) {
				current->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
			}
		}
		else if (l.begins_with("Tr ")) {
			// Transparency (1.0 is completely transparent, 0.0 is fully opaque).
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);
			Vector<String> v = l.split(" ", false);
			ERR_FAIL_COND_V(v.size() != 2, ERR_INVALID_DATA);
			float d = v[1].to_float();
			Color c = current->get_albedo();
			c.a = 1.0 - d;
			if (c.a < 0.99) {
				current->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
			}

		}
		else if (l.begins_with("map_Ka ")) {
			// Ambient texture map.
			WARN_PRINT(
				"OBJ: Ambient light texture for material '" + current_name + "' is ignored in PBR");

		}
		else if (l.begins_with("map_Kd ")) {
			// Diffuse texture map.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);

			String p = l.replace("map_Kd", "").replace_char('\\', '/').strip_edges();
			String path;
			if (p.is_absolute_path()) {
				path = p;
			}
			else {
				path = base_path.path_join(p);
			}

			Ref<Texture2D> texture = ResourceLoader::load(path);

			if (texture.is_valid()) {
				current->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, texture);
			}
			else if (r_missing_deps) {
				r_missing_deps->push_back(path);
			}

		}
		else if (l.begins_with("map_Ks ")) {
			// Specular color texture map.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);

			String p = l.replace("map_Ks", "").replace_char('\\', '/').strip_edges();
			String path;
			if (p.is_absolute_path()) {
				path = p;
			}
			else {
				path = base_path.path_join(p);
			}

			Ref<Texture2D> texture = ResourceLoader::load(path);

			if (texture.is_valid()) {
				current->set_texture(StandardMaterial3D::TEXTURE_METALLIC, texture);
			}
			else if (r_missing_deps) {
				r_missing_deps->push_back(path);
			}

		}
		else if (l.begins_with("map_Ns ")) {
			// Specular exponent texture map.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);

			String p = l.replace("map_Ns", "").replace_char('\\', '/').strip_edges();
			String path;
			if (p.is_absolute_path()) {
				path = p;
			}
			else {
				path = base_path.path_join(p);
			}

			Ref<Texture2D> texture = ResourceLoader::load(path);

			if (texture.is_valid()) {
				current->set_texture(StandardMaterial3D::TEXTURE_ROUGHNESS, texture);
			}
			else if (r_missing_deps) {
				r_missing_deps->push_back(path);
			}
		}
		else if (l.begins_with("map_bump ") || l.begins_with("map_Bump ")) {
			// Bump texture map.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);

			l = l.begins_with("map_bump ") ? l.trim_prefix("map_bump ")
										   : l.trim_prefix("map_Bump ");
			l = l.strip_edges();

			// Read path and optional bump multiplier.
			String p;
			float bm = 1.0;
			int bm_pos = l.find("-bm ");
			if (bm_pos >= 0) {
				int bm_start = bm_pos + 4;
				int bm_end = l.find_char(' ', bm_start);
				if (bm_end >= 0) {
					bm = l.substr(bm_start, bm_end - bm_start).to_float();
					p = l.substr(bm_end + 1);
				}
				else { // Bump multiplier ends at end of line.
					bm = l.substr(bm_start).to_float();
					p = l.substr(0, bm_pos);
				}
			}
			else {
				p = l;
			}

			String path = base_path.path_join(p.replace_char('\\', '/').strip_edges());

			Ref<Texture2D> texture = ResourceLoader::load(path);

			if (texture.is_valid()) {
				current->set_feature(StandardMaterial3D::FEATURE_NORMAL_MAPPING, true);
				current->set_texture(StandardMaterial3D::TEXTURE_NORMAL, texture);
				current->set_normal_scale(bm);
			}
			else if (r_missing_deps) {
				r_missing_deps->push_back(path);
			}
		}
		else if (f->eof_reached()) {
			break;
		}
	}

	return OK;
}

void EditorOBJImporter::get_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("obj");
}

////////////////////////////////////////////////////

String ResourceImporterOBJ::get_importer_name() const { return "wavefront_obj"; }

String ResourceImporterOBJ::get_visible_name() const { return "OBJ as Mesh"; }

void ResourceImporterOBJ::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("obj");
}

String ResourceImporterOBJ::get_save_extension() const { return "mesh"; }

String ResourceImporterOBJ::get_resource_type() const { return "Mesh"; }

int ResourceImporterOBJ::get_format_version() const { return 1; }

int ResourceImporterOBJ::get_preset_count() const { return 0; }

String ResourceImporterOBJ::get_preset_name(int p_idx) const { return ""; }

void ResourceImporterOBJ::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}


