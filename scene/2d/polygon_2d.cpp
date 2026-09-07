/**************************************************************************/
/*  polygon_2d.cpp                                                        */
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
#include "core/math/geometry_2d.h"
#include "polygon_2d.h"
#include "scene/2d/skeleton_2d.h"
#include "servers/rendering/rendering_server.h"

#ifndef NAVIGATION_2D_DISABLED
#include "scene/resources/2d/navigation_mesh_source_geometry_data_2d.h"
#include "scene/resources/2d/navigation_polygon.h"
#include "servers/navigation_2d/navigation_server_2d.h"

RID Polygon2D::_navmesh_source_geometry_parser;
#endif // NAVIGATION_2D_DISABLED

#ifdef TOOLS_ENABLED

void Polygon2D::_edit_set_pivot(const Point2& p_pivot)
{
	set_position(get_transform().xform(p_pivot));
	set_offset(get_offset() - p_pivot);
}

Point2 Polygon2D::_edit_get_pivot() const { return Vector2(); }

bool Polygon2D::_edit_use_pivot() const { return true; }
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
Rect2 Polygon2D::_edit_get_rect() const
{
	if (rect_cache_dirty) {
		int l = polygon.size();
		const Vector2* r = polygon.ptr();
		item_rect = Rect2();
		for (int i = 0; i < l; i++) {
			Vector2 pos = r[i] + offset;
			if (i == 0) {
				item_rect.position = pos;
			}
			else {
				item_rect.expand_to(pos);
			}
		}
		rect_cache_dirty = false;
	}

	return item_rect;
}

bool Polygon2D::_edit_use_rect() const { return polygon.size() > 0; }

#endif // DEBUG_ENABLED

void Polygon2D::_skeleton_bone_setup_changed() { queue_redraw(); }

void Polygon2D::set_polygon(const Vector<Vector2>& p_polygon)
{
	polygon = p_polygon;
	rect_cache_dirty = true;
	queue_redraw();
}

Vector<Vector2> Polygon2D::get_polygon() const { return polygon; }

void Polygon2D::set_internal_vertex_count(int p_count) { internal_vertices = p_count; }

int Polygon2D::get_internal_vertex_count() const { return internal_vertices; }

void Polygon2D::set_uv(const Vector<Vector2>& p_uv)
{
	uv = p_uv;
	queue_redraw();
}

Vector<Vector2> Polygon2D::get_uv() const { return uv; }

void Polygon2D::set_color(const Color& p_color)
{
	color = p_color;
	queue_redraw();
}

Color Polygon2D::get_color() const { return color; }

void Polygon2D::set_vertex_colors(const Vector<Color>& p_colors)
{
	vertex_colors = p_colors;
	queue_redraw();
}

Vector<Color> Polygon2D::get_vertex_colors() const { return vertex_colors; }

void Polygon2D::set_texture(const Ref<Texture2D>& p_texture)
{
	texture = p_texture;
	queue_redraw();
}

Ref<Texture2D> Polygon2D::get_texture() const { return texture; }

void Polygon2D::set_texture_offset(const Vector2& p_offset)
{
	tex_ofs = p_offset;
	queue_redraw();
}

Vector2 Polygon2D::get_texture_offset() const { return tex_ofs; }

void Polygon2D::set_texture_rotation(real_t p_rot)
{
	tex_rot = p_rot;
	queue_redraw();
}

real_t Polygon2D::get_texture_rotation() const { return tex_rot; }

void Polygon2D::set_texture_scale(const Size2& p_scale)
{
	tex_scale = p_scale;
	queue_redraw();
}

Size2 Polygon2D::get_texture_scale() const { return tex_scale; }

void Polygon2D::set_invert(bool p_invert)
{
	invert = p_invert;
	queue_redraw();
}

bool Polygon2D::get_invert() const { return invert; }

void Polygon2D::set_antialiased(bool p_antialiased)
{
	antialiased = p_antialiased;
	queue_redraw();
}

bool Polygon2D::get_antialiased() const { return antialiased; }

void Polygon2D::set_invert_border(real_t p_invert_border)
{
	invert_border = p_invert_border;
	queue_redraw();
}

real_t Polygon2D::get_invert_border() const { return invert_border; }

void Polygon2D::set_offset(const Vector2& p_offset)
{
	offset = p_offset;
	rect_cache_dirty = true;
	queue_redraw();
}

Vector2 Polygon2D::get_offset() const { return offset; }

void Polygon2D::add_bone(const NodePath& p_path, const Vector<float>& p_weights)
{
	Bone bone;
	bone.path = p_path;
	bone.weights = p_weights;
	bone_weights.push_back(bone);
}

int Polygon2D::get_bone_count() const { return bone_weights.size(); }

NodePath Polygon2D::get_bone_path(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, bone_weights.size(), NodePath());
	return bone_weights[p_index].path;
}

const Vector<float>& Polygon2D::get_bone_weights(int p_index) const
{
	static const Vector<float> EMPTY;
	ERR_FAIL_INDEX_V(p_index, bone_weights.size(), EMPTY);
	return bone_weights[p_index].weights;
}

void Polygon2D::erase_bone(int p_idx)
{
	ERR_FAIL_INDEX(p_idx, bone_weights.size());
	bone_weights.remove_at(p_idx);
}

void Polygon2D::clear_bones() { bone_weights.clear(); }

void Polygon2D::set_bone_weights(int p_index, const Vector<float>& p_weights)
{
	ERR_FAIL_INDEX(p_index, bone_weights.size());
	bone_weights.write[p_index].weights = p_weights;
	queue_redraw();
}

void Polygon2D::set_bone_path(int p_index, const NodePath& p_path)
{
	ERR_FAIL_INDEX(p_index, bone_weights.size());
	bone_weights.write[p_index].path = p_path;
	queue_redraw();
}

void Polygon2D::set_skeleton(const NodePath& p_skeleton)
{
	if (skeleton == p_skeleton) {
		return;
	}
	skeleton = p_skeleton;
	queue_redraw();
}

NodePath Polygon2D::get_skeleton() const { return skeleton; }

#ifndef NAVIGATION_2D_DISABLED

#endif // NAVIGATION_2D_DISABLED

Polygon2D::Polygon2D() { mesh = RS::get_singleton()->mesh_create(); }

Polygon2D::~Polygon2D()
{
	// This will free the internally-allocated mesh instance, if allocated.
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->canvas_item_attach_skeleton(get_canvas_item(), RID());
	RS::get_singleton()->free_rid(mesh);
}


