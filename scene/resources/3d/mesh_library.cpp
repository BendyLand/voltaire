/**************************************************************************/
/*  mesh_library.cpp                                                      */
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

#include "mesh_library.h"
#include "scene/resources/texture.h"
#include "servers/rendering/rendering_server.h" // IWYU pragma: keep // Needed to bind RSE enums.

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/box_shape_3d.h"
#endif // PHYSICS_3D_DISABLED

bool MeshLibrary::_validate_index(int p_idx)
{
	if (unlikely(!item_map.has(p_idx))) {
		if (!init_property) {
			ERR_FAIL_V_MSG(
				false, vformat("Requested for nonexistent MeshLibrary item '%d'.", p_idx));
		}
		create_item(p_idx);
	}

	return true;
}

void MeshLibrary::set_item_name(int p_item, const String& p_name)
{
	if (_validate_index(p_item)) {
		item_map[p_item].name = p_name;
		emit_changed();
	}
}

void MeshLibrary::set_item_category(int p_item, const StringName& p_category)
{
	if (_validate_index(p_item)) {
		item_map[p_item].category = p_category;
		emit_changed();
	}
}

void MeshLibrary::set_item_mesh(int p_item, const Ref<Mesh>& p_mesh)
{
	if (_validate_index(p_item)) {
		item_map[p_item].mesh = p_mesh;
		emit_changed();
	}
}

void MeshLibrary::set_item_mesh_transform(int p_item, const Transform3D& p_transform)
{
	if (_validate_index(p_item)) {
		item_map[p_item].mesh_transform = p_transform;
		emit_changed();
	}
}

void MeshLibrary::set_item_mesh_cast_shadow(
	int p_item, RSE::ShadowCastingSetting p_shadow_casting_setting)
{
	if (_validate_index(p_item)) {
		item_map[p_item].mesh_cast_shadow = p_shadow_casting_setting;
		emit_changed();
	}
}

void MeshLibrary::set_item_navigation_mesh(int p_item, const Ref<NavigationMesh>& p_navigation_mesh)
{
	if (_validate_index(p_item)) {
		item_map[p_item].navigation_mesh = p_navigation_mesh;
		emit_changed();
	}
}

void MeshLibrary::set_item_navigation_mesh_transform(int p_item, const Transform3D& p_transform)
{
	if (_validate_index(p_item)) {
		item_map[p_item].navigation_mesh_transform = p_transform;
		emit_changed();
	}
}

void MeshLibrary::set_item_navigation_layers(int p_item, uint32_t p_navigation_layers)
{
	if (_validate_index(p_item)) {
		item_map[p_item].navigation_layers = p_navigation_layers;
		emit_changed();
	}
}

void MeshLibrary::set_item_preview(int p_item, const Ref<Texture2D>& p_preview)
{
	if (_validate_index(p_item)) {
		item_map[p_item].preview = p_preview;
		emit_changed();
	}
}

String MeshLibrary::get_item_name(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), "",
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].name;
}

StringName MeshLibrary::get_item_category(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), StringName(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].category;
}

Ref<Mesh> MeshLibrary::get_item_mesh(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), Ref<Mesh>(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].mesh;
}

Transform3D MeshLibrary::get_item_mesh_transform(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), Transform3D(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].mesh_transform;
}

RSE::ShadowCastingSetting MeshLibrary::get_item_mesh_cast_shadow(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), RSE::ShadowCastingSetting::SHADOW_CASTING_SETTING_ON,
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].mesh_cast_shadow;
}

#ifndef PHYSICS_3D_DISABLED
Vector<MeshLibrary::ShapeData> MeshLibrary::get_item_shapes(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), Vector<ShapeData>(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].shapes;
}
#endif // PHYSICS_3D_DISABLED

Ref<NavigationMesh> MeshLibrary::get_item_navigation_mesh(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), Ref<NavigationMesh>(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].navigation_mesh;
}

Transform3D MeshLibrary::get_item_navigation_mesh_transform(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), Transform3D(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].navigation_mesh_transform;
}

uint32_t MeshLibrary::get_item_navigation_layers(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), 0,
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].navigation_layers;
}

Ref<Texture2D> MeshLibrary::get_item_preview(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), Ref<Texture2D>(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].preview;
}

bool MeshLibrary::has_item(int p_item) const { return item_map.has(p_item); }

Vector<int> MeshLibrary::get_item_list() const
{
	Vector<int> ret;
	ret.resize(item_map.size());
	int idx = 0;
	for (const KeyValue<int, Item>& E : item_map) {
		ret.write[idx++] = E.key;
	}

	return ret;
}

int MeshLibrary::find_item_by_name(const String& p_name) const
{
	for (const KeyValue<int, Item>& E : item_map) {
		if (E.value.name == p_name) {
			return E.key;
		}
	}
	return -1;
}

int MeshLibrary::get_last_unused_item_id() const
{
	if (!item_map.size()) {
		return 0;
	}
	else {
		return item_map.back()->key() + 1;
	}
}

void MeshLibrary::reset_state() { clear(); }


