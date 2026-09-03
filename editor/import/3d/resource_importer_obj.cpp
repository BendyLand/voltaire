/**************************************************************************/
/*  resource_importer_obj.cpp                                             */
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

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "resource_importer_obj.h"
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
			current->set_albedo(c);
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
			current->set_albedo(c);
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
			current->set_albedo(c);
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


