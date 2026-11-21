/**************************************************************************/
/*  sprite_2d.cpp                                                         */
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

#include "core/config/engine.h"
#include "scene/main/viewport.h"
#include "servers/display/accessibility_server.h"
#include "sprite_2d.h"

#ifdef DEBUG_ENABLED
bool Sprite2D::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	return is_pixel_opaque(p_point);
}

Rect2 Sprite2D::_edit_get_rect() const { return get_rect(); }

bool Sprite2D::_edit_use_rect() const { return texture.is_valid(); }
#endif // DEBUG_ENABLED

Rect2 Sprite2D::get_anchorable_rect() const { return get_rect(); }

void Sprite2D::_get_rects(Rect2& r_src_rect, Rect2& r_dst_rect, bool& r_filter_clip_enabled) const
{
	Rect2 base_rect;

	if (region_enabled) {
		r_filter_clip_enabled = region_filter_clip_enabled;
		base_rect = region_rect;
	}
	else {
		r_filter_clip_enabled = false;
		base_rect = Rect2(0, 0, texture->get_width(), texture->get_height());
	}

	Size2 frame_size = base_rect.size / Size2(hframes, vframes);
	Point2 frame_offset = Point2(frame % hframes, frame / hframes);
	frame_offset *= frame_size;

	r_src_rect.size = frame_size;
	r_src_rect.position = base_rect.position + frame_offset;

	Point2 dest_offset = offset;
	if (centered) {
		dest_offset -= frame_size / 2;
	}

	if (get_viewport() && get_viewport()->is_snap_2d_transforms_to_pixel_enabled()) {
		dest_offset = (dest_offset + Point2(0.5, 0.5)).floor();
	}

	r_dst_rect = Rect2(dest_offset, frame_size);

	if (hflip) {
		r_dst_rect.size.x = -r_dst_rect.size.x;
	}
	if (vflip) {
		r_dst_rect.size.y = -r_dst_rect.size.y;
	}
}

Point2 Sprite2D::_get_rect_offset(const Size2i& p_size) const
{
	Point2 ofs = offset;
	if (centered) {
		ofs -= Size2(p_size) / 2;
	}

	if (get_viewport() && get_viewport()->is_snap_2d_transforms_to_pixel_enabled()) {
		ofs = (ofs + Point2(0.5, 0.5)).floor();
	}

	return ofs;
}

void Sprite2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		Rect2 dst_rect = get_rect();

		AccessibilityServer::update_set_role(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_IMAGE);
		AccessibilityServer::update_set_transform(ae, get_transform());
		AccessibilityServer::update_set_bounds(ae, dst_rect);
	} break;

	case NOTIFICATION_DRAW: {
		if (texture.is_null()) {
			return;
		}

		RID ci = get_canvas_item();

		Rect2 src_rect, dst_rect;
		bool filter_clip_enabled;
		_get_rects(src_rect, dst_rect, filter_clip_enabled);

		texture->draw_rect_region(
			ci, dst_rect, src_rect, Color(1, 1, 1), false, filter_clip_enabled);
	} break;
	}
}

Ref<Texture2D> Sprite2D::get_texture() const { return texture; }

bool Sprite2D::is_centered() const { return centered; }

Point2 Sprite2D::get_offset() const { return offset; }

bool Sprite2D::is_flipped_h() const { return hflip; }

bool Sprite2D::is_flipped_v() const { return vflip; }

bool Sprite2D::is_region_enabled() const { return region_enabled; }

void Sprite2D::set_region_rect(const Rect2& p_region_rect)
{
	if (region_rect == p_region_rect) {
		return;
	}

	region_rect = p_region_rect;

	if (region_enabled) {
		item_rect_changed();
	}
}

Rect2 Sprite2D::get_region_rect() const { return region_rect; }

bool Sprite2D::is_region_filter_clip_enabled() const { return region_filter_clip_enabled; }

int Sprite2D::get_frame() const { return frame; }

void Sprite2D::set_frame_coords(const Vector2i& p_coord)
{
	ERR_FAIL_INDEX(p_coord.x, hframes);
	ERR_FAIL_INDEX(p_coord.y, vframes);

	set_frame(p_coord.y * hframes + p_coord.x);
}

Vector2i Sprite2D::get_frame_coords() const { return Vector2i(frame % hframes, frame / hframes); }

int Sprite2D::get_vframes() const { return vframes; }

int Sprite2D::get_hframes() const { return hframes; }

bool Sprite2D::is_pixel_opaque(const Point2& p_point) const
{
	if (texture.is_null()) {
		return false;
	}

	if (texture->get_size().width == 0 || texture->get_size().height == 0) {
		return false;
	}

	Rect2 src_rect, dst_rect;
	bool filter_clip_enabled;
	_get_rects(src_rect, dst_rect, filter_clip_enabled);
	dst_rect.size = dst_rect.size.abs();

	if (!dst_rect.has_point(p_point)) {
		return false;
	}

	Vector2 q = (p_point - dst_rect.position) / dst_rect.size;
	if (hflip) {
		q.x = 1.0f - q.x;
	}
	if (vflip) {
		q.y = 1.0f - q.y;
	}
	q = q * src_rect.size + src_rect.position;
	TextureRepeat repeat_mode = get_texture_repeat_in_tree();
	bool is_repeat = repeat_mode == TEXTURE_REPEAT_ENABLED || repeat_mode == TEXTURE_REPEAT_MIRROR;
	bool is_mirrored_repeat = repeat_mode == TEXTURE_REPEAT_MIRROR;
	if (is_repeat) {
		int mirror_x = 0;
		int mirror_y = 0;
		if (is_mirrored_repeat) {
			mirror_x = (int)(q.x / texture->get_size().width);
			mirror_y = (int)(q.y / texture->get_size().height);
		}
		q.x = Math::fmod(q.x, texture->get_size().width);
		q.y = Math::fmod(q.y, texture->get_size().height);
		if (mirror_x % 2 == 1) {
			q.x = texture->get_size().width - q.x - 1;
		}
		if (mirror_y % 2 == 1) {
			q.y = texture->get_size().height - q.y - 1;
		}
	}
	else {
		q = q.min(texture->get_size() - Vector2(1, 1));
	}

	return texture->is_pixel_opaque((int)q.x, (int)q.y);
}

bool Sprite2D::is_editor_region_rect_draggable() const
{
	return hframes <= 1 && vframes <= 1 && region_enabled;
}

Rect2 Sprite2D::get_rect() const
{
	if (texture.is_null()) {
		return Rect2(0, 0, 1, 1);
	}

	Size2i s;

	if (region_enabled) {
		s = region_rect.size;
	}
	else {
		s = texture->get_size();
	}

	s = s / Point2(hframes, vframes);

	Point2 ofs = _get_rect_offset(s);

	if (s == Size2(0, 0)) {
		s = Size2(1, 1);
	}

	return Rect2(ofs, s);
}



void Sprite2D::set_frame(int) {}

// ==========================================================================
// Merged from scene/2d/animated_sprite_2d.cpp
// ==========================================================================

#include "animated_sprite_2d.h"
#include "core/config/engine.h"
#include "scene/main/viewport.h"
#include "servers/display/accessibility_server.h"

#ifdef DEBUG_ENABLED
Rect2 AnimatedSprite2D::_edit_get_rect() const { return _get_rect(); }

bool AnimatedSprite2D::_edit_use_rect() const
{
	if (frames.is_null() || !frames->has_animation(animation)) {
		return false;
	}
	if (frame < 0 || frame >= frames->get_frame_count(animation)) {
		return false;
	}

	Ref<Texture2D> t;
	if (!animation.is_empty()) {
		t = frames->get_frame_texture(animation, frame);
	}
	return t.is_valid();
}
#endif // DEBUG_ENABLED

Rect2 AnimatedSprite2D::get_anchorable_rect() const { return _get_rect(); }

Rect2 AnimatedSprite2D::_get_rect() const
{
	if (frames.is_null() || !frames->has_animation(animation)) {
		return Rect2();
	}
	if (frame < 0 || frame >= frames->get_frame_count(animation)) {
		return Rect2();
	}

	Ref<Texture2D> t;
	if (!animation.is_empty()) {
		t = frames->get_frame_texture(animation, frame);
	}
	if (t.is_null()) {
		return Rect2();
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

Ref<SpriteFrames> AnimatedSprite2D::get_sprite_frames() const { return frames; }

int AnimatedSprite2D::get_frame() const { return frame; }

void AnimatedSprite2D::set_frame_progress(real_t p_progress) { frame_progress = p_progress; }

real_t AnimatedSprite2D::get_frame_progress() const { return frame_progress; }

void AnimatedSprite2D::set_speed_scale(float p_speed_scale) { speed_scale = p_speed_scale; }

float AnimatedSprite2D::get_speed_scale() const { return speed_scale; }

float AnimatedSprite2D::get_playing_speed() const
{
	if (!playing) {
		return 0;
	}
	return speed_scale * custom_speed_scale;
}

bool AnimatedSprite2D::is_centered() const { return centered; }

Point2 AnimatedSprite2D::get_offset() const { return offset; }

bool AnimatedSprite2D::is_flipped_h() const { return hflip; }

bool AnimatedSprite2D::is_flipped_v() const { return vflip; }

bool AnimatedSprite2D::is_playing() const { return playing; }

void AnimatedSprite2D::set_autoplay(const String& p_name)
{
	if (is_inside_tree() && !Engine::is_editor_hint()) {
		WARN_PRINT("Setting autoplay after the node has been added to the scene has no effect.");
	}

	autoplay = p_name;
}

String AnimatedSprite2D::get_autoplay() const { return autoplay; }

void AnimatedSprite2D::pause() { _stop_internal(false); }

void AnimatedSprite2D::stop() { _stop_internal(true); }

double AnimatedSprite2D::_get_frame_duration()
{
	if (frames.is_valid() && frames->has_animation(animation)) {
		return frames->get_frame_duration(animation, frame);
	}
	return 1.0;
}

void AnimatedSprite2D::_calc_frame_speed_scale()
{
	frame_speed_scale = 1.0 / _get_frame_duration();
}

String AnimatedSprite2D::get_animation() const { return animation; }

PackedStringArray AnimatedSprite2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (frames.is_null()) {
		warnings.push_back(
			RTR("A SpriteFrames resource must be created or set in the \"Sprite Frames\" property "
				"in order for AnimatedSprite2D to display frames."));
	}
	return warnings;
}

#ifdef TOOLS_ENABLED
void AnimatedSprite2D::get_argument_options(
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
	Node2D::get_argument_options(p_function, p_idx, r_options);
}
#endif // TOOLS_ENABLED

AnimatedSprite2D::AnimatedSprite2D() {}



void AnimatedSprite2D::_stop_internal(bool) {}

// ==========================================================================
// Merged from scene/2d/mesh_instance_2d.cpp
// ==========================================================================

#include "mesh_instance_2d.h"

#ifndef NAVIGATION_2D_DISABLED
#include <thirdparty/clipper2/include/clipper2/clipper.h>
#include "scene/resources/2d/navigation_mesh_source_geometry_data_2d.h"
#include "scene/resources/2d/navigation_polygon.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#endif // NAVIGATION_2D_DISABLED

RID MeshInstance2D::_navmesh_source_geometry_parser;

void MeshInstance2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		if (mesh.is_valid()) {
			draw_mesh(mesh.ptr(), texture);
		}
	} break;
	}
}





Ref<Mesh> MeshInstance2D::get_mesh() const { return mesh; }



Ref<Texture2D> MeshInstance2D::get_texture() const { return texture; }

#ifdef DEBUG_ENABLED
Rect2 MeshInstance2D::_edit_get_rect() const
{
	if (mesh.is_valid()) {
		AABB aabb = mesh->get_aabb();
		return Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);
	}

	return Node2D::_edit_get_rect();
}

bool MeshInstance2D::_edit_use_rect() const { return mesh.is_valid(); }
#endif // DEBUG_ENABLED

PackedStringArray MeshInstance2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (mesh.is_null()) {
		warnings.push_back(RTR("MeshInstance2D requires a Mesh to render anything. Please add a "
							   "mesh resource for it!"));
	}
	return warnings;
}

// ==========================================================================
// Merged from scene/2d/multimesh_instance_2d.cpp
// ==========================================================================

#include "multimesh_instance_2d.h"

#ifndef NAVIGATION_2D_DISABLED
#include <thirdparty/clipper2/include/clipper2/clipper.h>
#include "scene/resources/2d/navigation_mesh_source_geometry_data_2d.h"
#include "scene/resources/2d/navigation_polygon.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#endif // NAVIGATION_2D_DISABLED

RID MultiMeshInstance2D::_navmesh_source_geometry_parser;

void MultiMeshInstance2D::_refresh_interpolated()
{
	if (is_inside_tree() && multimesh.is_valid()) {
		bool interpolated = is_physics_interpolated_and_enabled();
		multimesh->set_physics_interpolated(interpolated);
	}
}

void MultiMeshInstance2D::_physics_interpolated_changed()
{
	CanvasItem::_physics_interpolated_changed();
	_refresh_interpolated();
}

void MultiMeshInstance2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_refresh_interpolated();
		break;
	}
	case NOTIFICATION_DRAW: {
		if (multimesh.is_valid()) {
			draw_multimesh(multimesh.ptr(), texture);
		}
	} break;
	}
}

Ref<MultiMesh> MultiMeshInstance2D::get_multimesh() const { return multimesh; }

Ref<Texture2D> MultiMeshInstance2D::get_texture() const { return texture; }

#ifdef DEBUG_ENABLED
Rect2 MultiMeshInstance2D::_edit_get_rect() const
{
	if (multimesh.is_valid()) {
		AABB aabb = multimesh->get_aabb();
		return Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);
	}

	return Node2D::_edit_get_rect();
}
#endif // DEBUG_ENABLED

// ==========================================================================
// Merged from scene/2d/polygon_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "polygon_2d.h"
#include "scene/2d/skeleton_2d.h"
#include "servers/rendering/renderer.h"

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

Vector<Vector2> Polygon2D::get_polygon() const { return polygon; }

void Polygon2D::set_internal_vertex_count(int p_count) { internal_vertices = p_count; }

int Polygon2D::get_internal_vertex_count() const { return internal_vertices; }

Vector<Vector2> Polygon2D::get_uv() const { return uv; }

Color Polygon2D::get_color() const { return color; }

Vector<Color> Polygon2D::get_vertex_colors() const { return vertex_colors; }

Ref<Texture2D> Polygon2D::get_texture() const { return texture; }

Vector2 Polygon2D::get_texture_offset() const { return tex_ofs; }

real_t Polygon2D::get_texture_rotation() const { return tex_rot; }

Size2 Polygon2D::get_texture_scale() const { return tex_scale; }

bool Polygon2D::get_invert() const { return invert; }

bool Polygon2D::get_antialiased() const { return antialiased; }

real_t Polygon2D::get_invert_border() const { return invert_border; }

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

NodePath Polygon2D::get_skeleton() const { return skeleton; }

Polygon2D::Polygon2D() { mesh = Renderer::mesh_create(); }

Polygon2D::~Polygon2D()
{
	// This will free the internally-allocated mesh instance, if allocated.
	ERR_FAIL_NULL(Renderer::data);
	Renderer::canvas_item_attach_skeleton(get_canvas_item(), RID());
	Renderer::free_rid(mesh);
}

void Polygon2D::set_offset(const Vector2& p_offset) {}

bool Polygon2D::_edit_is_selected_on_click(const Vector2& p_point, double p_tolerance) const
{
	return false;
}

// ==========================================================================
// Merged from scene/2d/skeleton_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/math/transform_interpolator.h"
#include "servers/rendering/renderer.h"
#include "skeleton_2d.h"

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
#endif // TOOLS_ENABLED

#ifdef TOOLS_ENABLED
bool Bone2D::_editor_get_show_bone_gizmo() const { return _editor_show_bone_gizmo; }
#endif // TOOLS_ENABLED

Transform2D Bone2D::get_rest() const { return rest; }

Transform2D Bone2D::get_skeleton_rest() const
{
	if (parent_bone) {
		return parent_bone->get_skeleton_rest() * rest;
	}
	else {
		return rest;
	}
}

void Bone2D::apply_rest() { set_transform(rest); }

int Bone2D::get_index_in_skeleton() const
{
	ERR_FAIL_NULL_V(skeleton, -1);
	skeleton->_update_bone_setup();
	return skeleton_index;
}

PackedStringArray Bone2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (!skeleton) {
		if (parent_bone) {
			warnings.push_back(RTR("This Bone2D chain should end at a Skeleton2D node."));
		}
		else {
			warnings.push_back(
				RTR("A Bone2D only works with a Skeleton2D or another Bone2D as parent node."));
		}
	}

	if (rest == Transform2D(0, 0, 0, 0, 0, 0)) {
		warnings.push_back(
			RTR("This bone lacks a proper REST pose. Go to the Skeleton2D node and set one."));
	}

	return warnings;
}

bool Bone2D::get_autocalculate_length_and_angle() const { return autocalculate_length_and_angle; }

real_t Bone2D::get_length() const { return length; }

real_t Bone2D::get_bone_angle() const { return bone_angle; }

Bone2D::Bone2D()
{
	skeleton = nullptr;
	parent_bone = nullptr;
	skeleton_index = -1;
	length = 16;
	bone_angle = 0;
	autocalculate_length_and_angle = true;
	set_notify_local_transform(true);
	set_hide_clip_children(true);
	// this is a clever hack so the bone knows no rest has been set yet, allowing to show an error.
	for (int i = 0; i < 3; i++) {
		rest[i] = Vector2(0, 0);
	}
	copy_transform_to_cache = true;
}

Bone2D::~Bone2D()
{
#ifdef TOOLS_ENABLED
	if (!editor_gizmo_rid.is_null()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(editor_gizmo_rid);
	}
#endif // TOOLS_ENABLED
}

//////////////////////////////////////

void Skeleton2D::_update_transform()
{
	if (bone_setup_dirty) {
		_update_bone_setup();
		return; // above will update transform anyway
	}
	if (!transform_dirty) {
		return;
	}

	transform_dirty = false;

	for (uint32_t i = 0; i < bones.size(); i++) {
		ERR_CONTINUE(bones[i].parent_index >= (int)i);
		if (bones[i].parent_index >= 0) {
			bones[i].accum_transform =
				bones[bones[i].parent_index].accum_transform * bones[i].bone->get_transform();
		}
		else {
			bones[i].accum_transform = bones[i].bone->get_transform();
		}
	}

	for (uint32_t i = 0; i < bones.size(); i++) {
		Transform2D final_xform = bones[i].accum_transform * bones[i].rest_inverse;
		Renderer::skeleton_bone_set_transform_2d(skeleton, i, final_xform);
	}
}

int Skeleton2D::get_bone_count() const
{
	ERR_FAIL_COND_V(!is_inside_tree(), 0);

	if (bone_setup_dirty) {
		// TODO: Is this necessary? It doesn't seem to change bones.size()
		const_cast<Skeleton2D*>(this)->_update_bone_setup();
	}

	return bones.size();
}

Bone2D* Skeleton2D::get_bone(int p_idx)
{
	ERR_FAIL_COND_V(!is_inside_tree(), nullptr);
	ERR_FAIL_INDEX_V(p_idx, (int)bones.size(), nullptr);

	return bones[p_idx].bone;
}

void Skeleton2D::_update_process_mode()
{
	bool process = modification_stack.is_valid() && is_inside_tree();
	if (!process) {
		// We might have another reason to process.
		process = is_physics_interpolated_and_enabled() && is_visible_in_tree();
	}

	set_process_internal(process);
	set_physics_process_internal(process);
}

void Skeleton2D::_ensure_update_interpolation_data()
{
	uint64_t tick = Engine::get_physics_frames();

	if (_interpolation_data.last_update_physics_tick != tick) {
		_interpolation_data.xform_prev = _interpolation_data.xform_curr;
		_interpolation_data.last_update_physics_tick = tick;
	}
}

void Skeleton2D::_physics_interpolated_changed() { _update_process_mode(); }

void Skeleton2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		if (bone_setup_dirty) {
			_update_bone_setup();
		}
		if (transform_dirty) {
			_update_transform();
		}
		request_ready();
	} break;

	case NOTIFICATION_ENTER_TREE: {
		_update_process_mode();

		if (is_physics_interpolated_and_enabled()) {
			_interpolation_data.xform_curr = get_global_transform();
			_interpolation_data.xform_prev = _interpolation_data.xform_curr;
		}
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		if (is_physics_interpolated_and_enabled()) {
			_ensure_update_interpolation_data();
			if (Engine::is_in_physics_frame()) {
				_interpolation_data.xform_curr = get_global_transform();
			}
		}
		else {
			Renderer::skeleton_set_base_transform_2d(skeleton, get_global_transform());
		}
	} break;

	case NOTIFICATION_RESET_PHYSICS_INTERPOLATION: {
		_interpolation_data.xform_curr = get_global_transform();
		_interpolation_data.xform_prev = _interpolation_data.xform_curr;
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		if (is_physics_interpolated_and_enabled()) {
			Transform2D res;
			TransformInterpolator::interpolate_transform_2d(_interpolation_data.xform_prev,
				_interpolation_data.xform_curr, res,
				Engine::get_physics_interpolation_fraction());
			Renderer::skeleton_set_base_transform_2d(skeleton, res);
		}
		if (modification_stack.is_valid()) {
			execute_modifications(get_process_delta_time(),
				SkeletonModificationStack2D::EXECUTION_MODE::execution_mode_process);
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (is_physics_interpolated_and_enabled()) {
			_ensure_update_interpolation_data();
			_interpolation_data.xform_curr = get_global_transform();
		}
		if (modification_stack.is_valid()) {
			execute_modifications(get_physics_process_delta_time(),
				SkeletonModificationStack2D::EXECUTION_MODE::execution_mode_physics_process);
		}
	} break;

	case NOTIFICATION_POST_ENTER_TREE: {
		set_modification_stack(modification_stack);
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		_update_process_mode();
	} break;

#ifdef TOOLS_ENABLED
	case NOTIFICATION_DRAW: {
		if (Engine::is_editor_hint()) {
			if (modification_stack.is_valid()) {
				modification_stack->draw_editor_gizmos();
			}
		}
	} break;
#endif // TOOLS_ENABLED
	}
}

RID Skeleton2D::get_skeleton() const { return skeleton; }

void Skeleton2D::set_bone_local_pose_override(
	int p_bone_idx, Transform2D p_override, real_t p_amount, bool p_persistent)
{
	ERR_FAIL_INDEX_MSG(p_bone_idx, (int)bones.size(), "Bone index is out of range!");
	bones[p_bone_idx].local_pose_override = p_override;
	bones[p_bone_idx].local_pose_override_amount = p_amount;
	bones[p_bone_idx].local_pose_override_persistent = p_persistent;
}

Transform2D Skeleton2D::get_bone_local_pose_override(int p_bone_idx)
{
	ERR_FAIL_INDEX_V_MSG(
		p_bone_idx, (int)bones.size(), Transform2D(), "Bone index is out of range!");
	return bones[p_bone_idx].local_pose_override;
}

void Skeleton2D::set_modification_stack(Ref<SkeletonModificationStack2D> p_stack)
{
	if (modification_stack.is_valid()) {
		modification_stack->is_setup = false;
		modification_stack->set_skeleton(nullptr);
	}
	modification_stack = p_stack;
	if (modification_stack.is_valid() && is_inside_tree()) {
		modification_stack->set_skeleton(this);
		modification_stack->setup();

#ifdef TOOLS_ENABLED
		modification_stack->set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
	}
	_update_process_mode();
}

Ref<SkeletonModificationStack2D> Skeleton2D::get_modification_stack() const
{
	return modification_stack;
}

Skeleton2D::Skeleton2D()
{
	skeleton = Renderer::skeleton_create();
	set_notify_transform(true);
	set_hide_clip_children(true);
}

Skeleton2D::~Skeleton2D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(skeleton);
}

void Skeleton2D::_update_bone_setup() {}
