/**************************************************************************/
/*  editor_import_collada.cpp                                             */
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
#include "core/io/resource_loader.h"
#include "core/templates/rb_set.h"
#include "editor/import/3d/collada.h"
#include "editor_import_collada.h"
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
				material->set_albedo(Color(1, 1, 1, 1));
				// material->set_parameter(StandardMaterial3D::PARAM_DIFFUSE,Color(1,1,1,1));
			}
			else {
				missing_textures.push_back(texfile.get_file());
			}
		}
	}
	else {
		material->set_albedo(effect.diffuse.color);
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

/*********************************************************************************/
/*************************************** SCENE ***********************************/
/*********************************************************************************/

void EditorSceneFormatImporterCollada::get_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("dae");
}


