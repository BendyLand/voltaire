/**************************************************************************/
/*  viewport.cpp                                                          */
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

#include <execinfo.h>
#include <unistd.h>
#include "core/string/print_string.h"
#include "viewport.h"

STATIC_ASSERT_INCOMPLETE_TYPE(class, Renderer);

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/templates/pair.h"
#include "core/templates/sort_array.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/popup.h"
#include "scene/gui/scroll_container.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/mesh.h"
#include "scene/resources/text_line.h"
#include "servers/audio/audio_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/rendering_server_enums.h"

// 2D.
#include "scene/2d/camera_2d.h"
#include "scene/resources/world_2d.h"

#ifndef _3D_DISABLED
#include "scene/3d/camera_3d.h"
#include "scene/resources/3d/world_3d.h"
#endif // _3D_DISABLED

#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_2D_DISABLED

#ifndef XR_DISABLED
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_server.h"
#endif // XR_DISABLED

void ViewportTexture::reset_local_to_scene()
{
	vp_changed = true;

	if (vp) {
		vp->viewport_textures.erase(this);
		vp = nullptr;
	}

	if (proxy.is_valid() && proxy_ph.is_null()) {
		proxy_ph = Renderer::texture_2d_placeholder_create();
		Renderer::texture_proxy_update(proxy, proxy_ph);
	}
}

void ViewportTexture::set_viewport_path_in_scene(const NodePath& p_path)
{
	if (path == p_path) {
		return;
	}

	path = p_path;

	reset_local_to_scene();

	if (get_local_scene() && !path.is_empty()) {
		setup_local_to_scene();
	}
	else {
		if (path.is_empty()) {
			vp_changed = false;
		}
		emit_changed();
	}
}

NodePath ViewportTexture::get_viewport_path_in_scene() const { return path; }

int ViewportTexture::get_width() const
{
	if (!vp) {
		_err_print_viewport_not_set();
		return 0;
	}
	if (vp->is_sub_viewport()) {
		return vp->size.width;
	}
	return vp->size.width * vp->get_stretch_transform().get_scale().width;
}

int ViewportTexture::get_height() const
{
	if (!vp) {
		_err_print_viewport_not_set();
		return 0;
	}
	if (vp->is_sub_viewport()) {
		return vp->size.height;
	}
	return vp->size.height * vp->get_stretch_transform().get_scale().height;
}

Size2 ViewportTexture::get_size() const
{
	if (!vp) {
		_err_print_viewport_not_set();
		return Size2();
	}
	if (vp->is_sub_viewport()) {
		return vp->size;
	}
	Size2 scale = vp->get_stretch_transform().get_scale();
	return Size2(vp->size.width * scale.width, vp->size.height * scale.height).ceil();
}

RID ViewportTexture::get_rid() const
{
	if (proxy.is_null()) {
		proxy_ph = Renderer::texture_2d_placeholder_create();
		proxy = Renderer::texture_proxy_create(proxy_ph);
	}
	return proxy;
}

bool ViewportTexture::has_alpha() const { return false; }

Ref<Image> ViewportTexture::get_image() const
{
	if (!vp) {
		_err_print_viewport_not_set();
		return Ref<Image>();
	}
	return Renderer::texture_2d_get(vp->texture_rid);
}

void ViewportTexture::_err_print_viewport_not_set() const
{
	if (!vp_pending && !vp_changed) {
		ERR_PRINT("Viewport Texture must be set to use it.");
	}
}

ViewportTexture::ViewportTexture() { set_local_to_scene(true); }

ViewportTexture::~ViewportTexture()
{
	if (vp) {
		vp->viewport_textures.erase(this);
	}

	ERR_FAIL_NULL(Renderer::data);

	if (proxy_ph.is_valid()) {
		Renderer::free_rid(proxy_ph);
	}
	if (proxy.is_valid()) {
		Renderer::free_rid(proxy);
	}
}

void Viewport::_sub_window_update_order()
{
	if (gui.sub_windows.size() < 2) {
		return;
	}

	// Reorder 'always on top' windows.
	int last_index = gui.sub_windows.size() - 1;
	for (int index = last_index, insert_index = last_index; index >= 0; index--) {
		SubWindow sw = gui.sub_windows[index];
		Window* parent_window = sw.window->get_parent_visible_window();
		bool parent_is_always_on_top =
			(parent_window != nullptr) && parent_window->get_flag(Window::FLAG_ALWAYS_ON_TOP);
		if (sw.window->get_flag(Window::FLAG_ALWAYS_ON_TOP) ||
			(parent_is_always_on_top && sw.window->is_exclusive())) {
			if (index != insert_index) {
				gui.sub_windows.remove_at(index);
				gui.sub_windows.insert(insert_index, sw);
			}
			insert_index--;
		}
	}

	// Reorder exclusive children.
	for (int parent_index = 0; parent_index < gui.sub_windows.size(); parent_index++) {
		Window* exclusive_child = gui.sub_windows[parent_index].window->get_exclusive_child();
		if (exclusive_child != nullptr && exclusive_child->is_visible()) {
			int child_index = _sub_window_find(exclusive_child);
			if (child_index < parent_index) {
				SubWindow sw = gui.sub_windows[child_index];
				gui.sub_windows.remove_at(child_index);
				gui.sub_windows.insert(parent_index, sw);
			}
		}
	}

	for (int i = 0; i < gui.sub_windows.size(); i++) {
		Renderer::canvas_item_set_draw_index(gui.sub_windows[i].canvas_item, i);
	}
}

void Viewport::_sub_window_register(Window* p_window)
{
	ERR_FAIL_COND(!is_inside_tree());
	for (int i = 0; i < gui.sub_windows.size(); i++) {
		ERR_FAIL_COND(gui.sub_windows[i].window == p_window);
	}

	if (gui.sub_windows.is_empty()) {
		subwindow_canvas = Renderer::canvas_create();
		Renderer::viewport_attach_canvas(viewport, subwindow_canvas);
		Renderer::viewport_set_canvas_stacking(viewport, subwindow_canvas, SUBWINDOW_CANVAS_LAYER, 0);
	}
	SubWindow sw;
	sw.canvas_item = Renderer::canvas_item_create();
	Renderer::canvas_item_set_parent(sw.canvas_item, subwindow_canvas);
	sw.window = p_window;
	gui.sub_windows.push_back(sw);

	if (gui.subwindow_drag == SUB_WINDOW_DRAG_DISABLED) {
		if (p_window->get_flag(Window::FLAG_NO_FOCUS)) {
			_sub_window_update_order();
		}
		else {
			_sub_window_grab_focus(p_window);
		}
	}
	else {
		int index = _sub_window_find(gui.currently_dragged_subwindow);
		sw = gui.sub_windows[index];
		gui.sub_windows.remove_at(index);
		gui.sub_windows.push_back(sw);
		_sub_window_update_order();
	}

	Renderer::viewport_set_parent_viewport(p_window->viewport, viewport);
}

void Viewport::_sub_window_update(Window* p_window)
{
	int index = _sub_window_find(p_window);

	// _sub_window_update is sometimes called deferred, and the window may have been closed since
	// then. For example, when the user resizes the game window. In that case, _sub_window_find will
	// not find it, which is expected.
	if (index == -1) {
		return;
	}

	SubWindow& sw = gui.sub_windows.write[index];
	sw.pending_window_update = false;

	Renderer::canvas_item_clear(sw.canvas_item);
	const Rect2i r = Rect2i(p_window->get_position(), p_window->get_size());

	if (!p_window->get_flag(Window::FLAG_BORDERLESS)) {
		TextServer::set_current_drawn_item_oversampling(get_oversampling());

		Ref<StyleBox> panel = gui.subwindow_focused == p_window
								  ? p_window->theme_cache.embedded_border
								  : p_window->theme_cache.embedded_unfocused_border;
		panel->draw(sw.canvas_item, r);

		// Draw the title bar text.
		Ref<Font> title_font = p_window->theme_cache.title_font;
		int font_size = p_window->theme_cache.title_font_size;
		Color title_color = p_window->theme_cache.title_color;
		int title_height = p_window->theme_cache.title_height;
		int close_h_ofs = p_window->theme_cache.close_h_offset;
		int close_v_ofs = p_window->theme_cache.close_v_offset;

		const real_t title_space = r.size.width - panel->get_minimum_size().x - close_h_ofs;
		if (title_space > 0) {
			TextLine title_text = TextLine(p_window->get_displayed_title(), title_font, font_size);
			title_text.set_width(title_space);
			title_text.set_direction(
				p_window->is_layout_rtl() ? TextServer::DIRECTION_RTL : TextServer::DIRECTION_LTR);
			int x = (r.size.width - title_text.get_size().x) / 2;
			int y = (-title_height - title_text.get_size().y) / 2;

			Color font_outline_color = p_window->theme_cache.title_outline_modulate;
			int outline_size = p_window->theme_cache.title_outline_size;
			if (outline_size > 0 && font_outline_color.a > 0) {
				title_text.draw_outline(
					sw.canvas_item, r.position + Point2(x, y), outline_size, font_outline_color);
			}
			title_text.draw(sw.canvas_item, r.position + Point2(x, y), title_color);
		}

		bool pressed = gui.subwindow_focused == sw.window &&
					   gui.subwindow_drag == SUB_WINDOW_DRAG_CLOSE &&
					   gui.subwindow_drag_close_inside;
		Ref<Texture2D> close_icon =
			pressed ? p_window->theme_cache.close_pressed : p_window->theme_cache.close;
		close_icon->draw(
			sw.canvas_item, r.position + Vector2(r.size.width - close_h_ofs, -close_v_ofs));

		TextServer::set_current_drawn_item_oversampling(0.0);
	}

	const Transform2D xform = sw.window->window_transform * sw.window->stretch_transform;
	Rect2 vr = xform.xform(sw.window->get_visible_rect());
	vr.position += p_window->get_position();
	if (vr != r) {
		Renderer::canvas_item_add_rect(sw.canvas_item, r, Color());
	}
	Renderer::canvas_item_add_texture_rect(sw.canvas_item, vr, sw.window->get_texture()->get_rid());
}

int Viewport::_sub_window_find(Window* p_window) const
{
	for (int i = 0; i < gui.sub_windows.size(); i++) {
		if (gui.sub_windows[i].window == p_window) {
			return i;
		}
	}

	return -1;
}

void Viewport::_update_viewport_path()
{
	if (!is_inside_tree()) {
		return;
	}

	for (ViewportTexture* E : viewport_textures) {
		Node* loc_scene = E->get_local_scene();
		if (loc_scene && loc_scene->is_inside_tree()) {
			E->path = loc_scene->get_path_to(this);
		}
	}
}

bool Viewport::_can_hide_focus_state()
{
	return Engine::is_editor_hint() ||
		   GLOBAL_GET_CACHED(int, "gui/common/show_focus_state_on_pointer_event") < 2;
}

RID Viewport::get_viewport_rid() const
{
	ERR_READ_THREAD_GUARD_V(RID());
	return viewport;
}

void Viewport::set_use_oversampling(bool p_oversampling)
{
	ERR_MAIN_THREAD_GUARD;
	if (use_font_oversampling == p_oversampling) {
		return;
	}
	use_font_oversampling = p_oversampling;
	_set_size(_get_size(), _get_view_count(), _get_size_2d_override(), _is_size_allocated());
}

bool Viewport::is_using_oversampling() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_font_oversampling;
}

void Viewport::set_oversampling_override(float p_oversampling)
{
	ERR_MAIN_THREAD_GUARD;
	if (font_oversampling_override == p_oversampling) {
		return;
	}
	font_oversampling_override = p_oversampling;
	_set_size(_get_size(), _get_view_count(), _get_size_2d_override(), _is_size_allocated());
}

float Viewport::get_oversampling_override() const
{
	ERR_READ_THREAD_GUARD_V(0.0);
	return font_oversampling_override;
}

void Viewport::_check_xr_size()
{
#ifndef XR_DISABLED
	// If our viewport has the use_xr flag set, our size and layout is managed by the XRServer.
	if (use_xr && XRServer::is_initialized()) {
		Ref<XRInterface> xr_interface = XRServer::get_primary_interface();
		if (xr_interface.is_valid() && xr_interface->is_initialized()) {
			Size2 xr_size = xr_interface->get_render_target_size();
			int xr_view_count = xr_interface->get_view_count();

			_set_size((Size2i)xr_size, xr_view_count, Size2i(0, 0), true);
		}
		else {
			// Set to default and prevent rendering for now (unless in editor, so we get a preview).
			bool is_editor = Engine::is_editor_hint();
			_set_size(is_editor ? Size2i(512, 512) : Size2i(0, 0), is_editor ? 1 : 0, Size2i(0, 0),
				false);
		}
	}
#endif // XR_DISABLED
}

Size2i Viewport::_get_size() const { return size; }

Size2 Viewport::_get_size_2d_override() const { return size_2d_override; }

int Viewport::_get_view_count() const { return view_count; }

bool Viewport::_is_size_allocated() const { return size_allocated; }

Rect2 Viewport::get_visible_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2());
	Rect2 r;

	if (size == Size2()) {
		r = Rect2(Point2(), DisplayServer::window_get_size());
	}
	else {
		r = Rect2(Point2(), size);
	}

	if (size_2d_override != Size2()) {
		r.size = size_2d_override;
	}
	return r;
}

void Viewport::set_canvas_transform(const Transform2D& p_transform)
{
	ERR_MAIN_THREAD_GUARD;
	canvas_transform = p_transform;

	Renderer::viewport_set_canvas_transform(
		viewport, find_world_2d()->get_canvas(), canvas_transform);
}

Transform2D Viewport::get_canvas_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return canvas_transform;
}

void Viewport::_update_global_transform()
{
	Transform2D sxform = stretch_transform * global_canvas_transform;

	Renderer::viewport_set_global_canvas_transform(viewport, sxform);
}

void Viewport::set_global_canvas_transform(const Transform2D& p_transform)
{
	ERR_MAIN_THREAD_GUARD;
	global_canvas_transform = p_transform;

	_update_global_transform();
}

Transform2D Viewport::get_global_canvas_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return global_canvas_transform;
}

void Viewport::_canvas_layer_add(CanvasLayer* p_canvas_layer)
{
	canvas_layers.insert(p_canvas_layer);
}

void Viewport::_canvas_layer_remove(CanvasLayer* p_canvas_layer)
{
	canvas_layers.erase(p_canvas_layer);
}

void Viewport::set_transparent_background(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	transparent_bg = p_enable;
	Renderer::viewport_set_transparent_background(viewport, p_enable);
}

bool Viewport::has_transparent_background() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return transparent_bg;
}

void Viewport::set_use_hdr_2d(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	use_hdr_2d = p_enable;
	Renderer::viewport_set_use_hdr_2d(viewport, p_enable);
}

bool Viewport::is_using_hdr_2d() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_hdr_2d;
}

void Viewport::set_world_2d(const Ref<World2D>& p_world_2d)
{
	ERR_MAIN_THREAD_GUARD;
	if (world_2d == p_world_2d) {
		return;
	}

	if (is_inside_tree()) {
		Renderer::viewport_remove_canvas(viewport, current_canvas);
	}

	if (world_2d.is_valid()) {
		world_2d->remove_viewport(this);
	}

	if (p_world_2d.is_valid()) {
		bool do_propagate = world_2d.is_valid() && is_inside_tree();
		world_2d = p_world_2d;
		if (do_propagate) {
			_propagate_world_2d_changed(this);
		}
	}
	else {
		WARN_PRINT("Invalid world_2d");
		world_2d.instantiate();
	}

	world_2d->register_viewport(this);
	_update_audio_listener_2d();

	if (is_inside_tree()) {
		current_canvas = find_world_2d()->get_canvas();
		Renderer::viewport_attach_canvas(viewport, current_canvas);
	}
}

Ref<World2D> Viewport::find_world_2d() const
{
	ERR_READ_THREAD_GUARD_V(Ref<World2D>());
	if (world_2d.is_valid()) {
		return world_2d;
	}
	else if (parent) {
		return parent->find_world_2d();
	}
	else {
		return Ref<World2D>();
	}
}

Ref<World2D> Viewport::get_world_2d() const
{
	ERR_READ_THREAD_GUARD_V(Ref<World2D>());
	return world_2d;
}

Transform2D Viewport::get_stretch_transform() const { return stretch_transform; }

Transform2D Viewport::get_final_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return stretch_transform * global_canvas_transform;
}

Ref<ViewportTexture> Viewport::get_texture() const
{
	ERR_READ_THREAD_GUARD_V(Ref<ViewportTexture>());
	return default_texture;
}

void Viewport::set_positional_shadow_atlas_size(int p_size)
{
	ERR_MAIN_THREAD_GUARD;
	positional_shadow_atlas_size = p_size;
	Renderer::viewport_set_positional_shadow_atlas_size(
		viewport, p_size, positional_shadow_atlas_16_bits);
}

int Viewport::get_positional_shadow_atlas_size() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return positional_shadow_atlas_size;
}

void Viewport::set_positional_shadow_atlas_16_bits(bool p_16_bits)
{
	ERR_MAIN_THREAD_GUARD;
	if (positional_shadow_atlas_16_bits == p_16_bits) {
		return;
	}

	positional_shadow_atlas_16_bits = p_16_bits;
	Renderer::viewport_set_positional_shadow_atlas_size(
		viewport, positional_shadow_atlas_size, positional_shadow_atlas_16_bits);
}

bool Viewport::get_positional_shadow_atlas_16_bits() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return positional_shadow_atlas_16_bits;
}

void Viewport::set_positional_shadow_atlas_quadrant_subdiv(
	int p_quadrant, PositionalShadowAtlasQuadrantSubdiv p_subdiv)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_quadrant, 4);
	ERR_FAIL_INDEX(p_subdiv, SHADOW_ATLAS_QUADRANT_SUBDIV_MAX);

	if (positional_shadow_atlas_quadrant_subdiv[p_quadrant] == p_subdiv) {
		return;
	}

	positional_shadow_atlas_quadrant_subdiv[p_quadrant] = p_subdiv;
	static const int subdiv[SHADOW_ATLAS_QUADRANT_SUBDIV_MAX] = {0, 1, 4, 16, 64, 256, 1024};

	Renderer::viewport_set_positional_shadow_atlas_quadrant_subdivision(
		viewport, p_quadrant, subdiv[p_subdiv]);
}

Viewport::PositionalShadowAtlasQuadrantSubdiv Viewport::get_positional_shadow_atlas_quadrant_subdiv(
	int p_quadrant) const
{
	ERR_READ_THREAD_GUARD_V(SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
	ERR_FAIL_INDEX_V(p_quadrant, 4, SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
	return positional_shadow_atlas_quadrant_subdiv[p_quadrant];
}

Vector2 Viewport::get_mouse_position() const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	if (get_section_root_viewport() != SceneTree::get_root()) {
		// Rely on the most recent mouse coordinate from an InputEventMouse in push_input.
		// In this case get_screen_transform is not applicable, because it is ambiguous.
		return gui.last_mouse_pos;
	}
	else if (DisplayServer::has_feature(DisplayServerEnums::FEATURE_MOUSE)) {
		Transform2D xform = get_screen_transform_internal(true);
		if (xform.determinant() == 0) {
			// Screen transform can be non-invertible when the Window is minimized.
			return Vector2();
		}
		return xform.affine_inverse().xform(DisplayServer::mouse_get_position());
	}
	else {
		// Fallback to Input for getting mouse position in case of emulated mouse.
		return get_screen_transform_internal().affine_inverse().xform(
			Input::get_mouse_position());
	}
}

void Viewport::warp_mouse(const Vector2& p_position)
{
	ERR_MAIN_THREAD_GUARD;
	Transform2D xform = get_screen_transform_internal();
	Vector2 gpos = xform.xform(p_position);
	Input::warp_mouse(gpos);
}

Point2 Viewport::wrap_mouse_in_rect(const Vector2& p_relative, const Rect2& p_rect)
{
	// Move the mouse cursor from its current position to a location bounded by `p_rect`
	// in accordance with a heuristic that takes the traveled distance `p_relative` of the mouse
	// into account.

	// All parameters are in viewport coordinates.
	// p_relative denotes the distance to the previous mouse position.
	// p_rect denotes the area, in which the mouse should be confined in.

	// The relative distance reported for the next event after a warp is in the boundaries of the
	// size of the rect on that axis, but it may be greater, in which case there's no problem as
	// fmod() will warp it, but if the pointer has moved in the opposite direction between the
	// pointer relocation and the subsequent event, the reported relative distance will be less
	// than the size of the rect and thus fmod() will be disabled for handling the situation.
	// And due to this mouse warping mechanism being stateless, we need to apply some heuristics
	// to detect the warp: if the relative distance is greater than the half of the size of the
	// relevant rect (checked per each axis), it will be considered as the consequence of a former
	// pointer warp.

	const Point2 rel_sign(p_relative.x >= 0.0f ? 1 : -1, p_relative.y >= 0.0 ? 1 : -1);
	const Size2 warp_margin = p_rect.size * 0.5f;
	const Point2 rel_warped(Math::fmod(p_relative.x + rel_sign.x * warp_margin.x, p_rect.size.x) -
								rel_sign.x * warp_margin.x,
		Math::fmod(p_relative.y + rel_sign.y * warp_margin.y, p_rect.size.y) -
			rel_sign.y * warp_margin.y);

	const Point2 pos_local = get_mouse_position() - p_rect.position;
	const Point2 pos_warped(
		Math::fposmod(pos_local.x, p_rect.size.x), Math::fposmod(pos_local.y, p_rect.size.y));
	if (pos_warped != pos_local) {
		warp_mouse(pos_warped + p_rect.position);
	}

	return rel_warped;
}

void Viewport::_gui_cancel_tooltip()
{
	gui.tooltip_control = nullptr;
	gui.tooltip_text = "";

	if (gui.tooltip_timer.is_valid()) {
		gui.tooltip_timer->release_connections();
		gui.tooltip_timer = Ref<SceneTreeTimer>();
	}
	if (gui.tooltip_popup) {
		gui.tooltip_popup->queue_free();
	}
}

void Viewport::cancel_tooltip() { _gui_cancel_tooltip(); }

void Viewport::show_tooltip(Control* p_control)
{
	if (!p_control) {
		return;
	}

	if (gui.tooltip_timer.is_valid()) {
		gui.tooltip_timer->release_connections();
		gui.tooltip_timer = Ref<SceneTreeTimer>();
	}
	gui.tooltip_control = p_control;
	_gui_show_tooltip_at(p_control->get_size() / 2);
}

void Viewport::_gui_show_tooltip() { _gui_show_tooltip_at(gui.last_mouse_pos); }

// `gui_find_control` doesn't take embedded windows into account. So the caller of this function
// needs to make sure, that there is no embedded window at the specified position.
Control* Viewport::gui_find_control(const Point2& p_global)
{
	ERR_MAIN_THREAD_GUARD_V(nullptr);

	_gui_sort_roots();

	for (List<Control*>::Element* E = gui.roots.back(); E; E = E->prev()) {
		Control* sw = E->get();
		if (!sw->is_visible_in_tree()) {
			continue;
		}

		Transform2D xform;
		CanvasItem* pci = sw->get_parent_item();
		if (pci) {
			xform = pci->get_global_transform_with_canvas();
		}
		else {
			xform = sw->get_canvas_transform();
		}

		Control* ret = _gui_find_control_at_pos(sw, p_global, xform);
		if (ret) {
			return ret;
		}
	}

	return nullptr;
}

void Viewport::_perform_drop(Control* p_control)
{
	gui_perform_drop_at(p_control ? p_control->get_local_mouse_position() : Vector2(), p_control);
}

List<Control*>::Element* Viewport::_gui_add_root_control(Control* p_control)
{
	gui.roots_order_dirty = true;
	return gui.roots.push_back(p_control);
}

void Viewport::gui_set_root_order_dirty()
{
	ERR_MAIN_THREAD_GUARD;
	gui.roots_order_dirty = true;
}

void Viewport::_gui_force_drag_start()
{
	Viewport* section_root = get_section_root_viewport();
	section_root->gui.global_dragging = true;
}

void Viewport::_gui_force_drag_cancel()
{
	Viewport* section_root = get_section_root_viewport();
	section_root->gui.global_dragging = false;
}

void Viewport::_gui_remove_root_control(List<Control*>::Element* RI) { gui.roots.erase(RI); }

void Viewport::_gui_unfocus_control(Control* p_control)
{
	if (gui.key_focus == p_control) {
		gui.key_focus->release_focus();
	}
}

void Viewport::canvas_item_top_level_changed() { _gui_update_mouse_over(); }

void Viewport::_gui_remove_focus_for_window(Node* p_window)
{
	if (get_base_window() == p_window) {
		gui_release_focus();
	}
}

bool Viewport::_gui_control_has_focus(const Control* p_control, bool p_ignore_hidden_focus)
{
	return (!p_ignore_hidden_focus || !gui.hide_focus) && gui.key_focus == p_control;
}

void Viewport::_gui_accept_event()
{
	if (is_inside_tree()) {
		set_input_as_handled();
	}
}

void Viewport::push_text_input(const String& p_text) { _push_text_input(p_text, false); }

Viewport::SubWindowResize Viewport::_sub_window_get_resize_margin(
	Window* p_subwindow, const Point2& p_point)
{
	if (p_subwindow->get_flag(Window::FLAG_BORDERLESS) ||
		p_subwindow->get_flag(Window::FLAG_RESIZE_DISABLED)) {
		return SUB_WINDOW_RESIZE_DISABLED;
	}

	Rect2i r = Rect2i(p_subwindow->get_position(), p_subwindow->get_size());

	int title_height = p_subwindow->theme_cache.title_height;

	r.position.y -= title_height;
	r.size.y += title_height;

	if (r.has_point(p_point)) {
		return SUB_WINDOW_RESIZE_DISABLED; // It's inside, so no resize.
	}

	int dist_x =
		p_point.x < r.position.x
			? (p_point.x - r.position.x)
			: (p_point.x > (r.position.x + r.size.x) ? (p_point.x - (r.position.x + r.size.x)) : 0);
	int dist_y =
		p_point.y < r.position.y
			? (p_point.y - r.position.y)
			: (p_point.y > (r.position.y + r.size.y) ? (p_point.y - (r.position.y + r.size.y)) : 0);

	int limit = p_subwindow->theme_cache.resize_margin;

	if (Math::abs(dist_x) > limit) {
		return SUB_WINDOW_RESIZE_DISABLED;
	}

	if (Math::abs(dist_y) > limit) {
		return SUB_WINDOW_RESIZE_DISABLED;
	}

	if (dist_x < 0 && dist_y < 0) {
		return SUB_WINDOW_RESIZE_TOP_LEFT;
	}

	if (dist_x == 0 && dist_y < 0) {
		return SUB_WINDOW_RESIZE_TOP;
	}

	if (dist_x > 0 && dist_y < 0) {
		return SUB_WINDOW_RESIZE_TOP_RIGHT;
	}

	if (dist_x < 0 && dist_y == 0) {
		return SUB_WINDOW_RESIZE_LEFT;
	}

	if (dist_x > 0 && dist_y == 0) {
		return SUB_WINDOW_RESIZE_RIGHT;
	}

	if (dist_x < 0 && dist_y > 0) {
		return SUB_WINDOW_RESIZE_BOTTOM_LEFT;
	}

	if (dist_x == 0 && dist_y > 0) {
		return SUB_WINDOW_RESIZE_BOTTOM;
	}

	if (dist_x > 0 && dist_y > 0) {
		return SUB_WINDOW_RESIZE_BOTTOM_RIGHT;
	}

	return SUB_WINDOW_RESIZE_DISABLED;
}

bool Viewport::_sub_windows_forward_input(const InputEvent& p_event)
{
	if (gui.subwindow_drag != SUB_WINDOW_DRAG_DISABLED) {
		ERR_FAIL_NULL_V(gui.currently_dragged_subwindow, false);

		if (p_event.type == InputEventType::MOUSE_BUTTON && !p_event.is_pressed() &&
			p_event.mouse_button.button_index == MouseButton::LEFT) {
			if (gui.subwindow_drag == SUB_WINDOW_DRAG_CLOSE) {
				if (gui.subwindow_drag_close_rect.has_point(p_event.mouse_button.position)) {
					// Close window.
					gui.currently_dragged_subwindow->_event_callback(
						DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
				}
			}
			gui.subwindow_drag = SUB_WINDOW_DRAG_DISABLED;
			if (gui.currently_dragged_subwindow != nullptr) { // May have been erased.
				_sub_window_update(gui.currently_dragged_subwindow);
				gui.currently_dragged_subwindow = nullptr;
			}
		}

		if (p_event.type == InputEventType::MOUSE_MOTION) {
			const Vector2 mm_pos = p_event.mouse_motion.position;
			if (gui.subwindow_drag == SUB_WINDOW_DRAG_MOVE) {
				Vector2 diff = mm_pos - gui.subwindow_drag_from;
				Rect2i new_rect(
					gui.subwindow_drag_pos + diff, gui.currently_dragged_subwindow->get_size());

				if (gui.currently_dragged_subwindow->is_clamped_to_embedder()) {
					new_rect = gui.currently_dragged_subwindow->fit_rect_in_parent(
						new_rect, get_visible_rect());
				}

				gui.currently_dragged_subwindow->_rect_changed_callback(new_rect);

				if (DisplayServer::has_feature(DisplayServerEnums::FEATURE_CURSOR_SHAPE)) {
					DisplayServer::cursor_set_shape(DisplayServerEnums::CURSOR_MOVE);
				}
			}
			if (gui.subwindow_drag == SUB_WINDOW_DRAG_CLOSE) {
				gui.subwindow_drag_close_inside = gui.subwindow_drag_close_rect.has_point(mm_pos);
			}
			if (gui.subwindow_drag == SUB_WINDOW_DRAG_RESIZE) {
				Vector2i diff = mm_pos - gui.subwindow_drag_from;
				Size2i min_size = gui.currently_dragged_subwindow->get_min_size();
				Size2i min_size_clamped =
					gui.currently_dragged_subwindow->get_clamped_minimum_size();

				min_size_clamped = min_size_clamped.maxi(1);

				Rect2i r = gui.subwindow_resize_from_rect;

				Size2i limit = r.size - min_size_clamped;

				switch (gui.subwindow_resize_mode) {
				case SUB_WINDOW_RESIZE_TOP_LEFT: {
					diff.x = MIN(diff.x, limit.x);
					diff.y = MIN(diff.y, limit.y);
					r.position += diff;
					r.size -= diff;
				} break;
				case SUB_WINDOW_RESIZE_TOP: {
					diff.x = 0;
					diff.y = MIN(diff.y, limit.y);
					r.position += diff;
					r.size -= diff;
				} break;
				case SUB_WINDOW_RESIZE_TOP_RIGHT: {
					diff.x = MAX(diff.x, -limit.x);
					diff.y = MIN(diff.y, limit.y);
					r.position.y += diff.y;
					r.size.y -= diff.y;
					r.size.x += diff.x;
				} break;
				case SUB_WINDOW_RESIZE_LEFT: {
					diff.x = MIN(diff.x, limit.x);
					diff.y = 0;
					r.position += diff;
					r.size -= diff;

				} break;
				case SUB_WINDOW_RESIZE_RIGHT: {
					diff.x = MAX(diff.x, -limit.x);
					r.size.x += diff.x;
				} break;
				case SUB_WINDOW_RESIZE_BOTTOM_LEFT: {
					diff.x = MIN(diff.x, limit.x);
					diff.y = MAX(diff.y, -limit.y);
					r.position.x += diff.x;
					r.size.x -= diff.x;
					r.size.y += diff.y;

				} break;
				case SUB_WINDOW_RESIZE_BOTTOM: {
					diff.y = MAX(diff.y, -limit.y);
					r.size.y += diff.y;
				} break;
				case SUB_WINDOW_RESIZE_BOTTOM_RIGHT: {
					diff.x = MAX(diff.x, -limit.x);
					diff.y = MAX(diff.y, -limit.y);
					r.size += diff;

				} break;
				default: {
				}
				}

				Size2i max_size = gui.currently_dragged_subwindow->get_max_size();
				if ((max_size.x > 0 || max_size.y > 0) &&
					(max_size.x >= min_size.x && max_size.y >= min_size.y)) {
					max_size = max_size.maxi(1);

					if (r.size.x > max_size.x) {
						r.size.x = max_size.x;
					}
					if (r.size.y > max_size.y) {
						r.size.y = max_size.y;
					}
				}

				gui.currently_dragged_subwindow->_rect_changed_callback(r);
			}

			if (gui.currently_dragged_subwindow) { // May have been erased.
				_sub_window_update(gui.currently_dragged_subwindow);
			}
		}

		return true; // Handled.
	}
	// If the event is a mouse button, we need to check whether another window was clicked.
	if (p_event.type == InputEventType::MOUSE_BUTTON && p_event.is_pressed() &&
		p_event.mouse_button.button_index == MouseButton::LEFT) {
		const Vector2 mb_pos = p_event.mouse_button.position;
		Window* click_on_window = nullptr;
		for (int i = gui.sub_windows.size() - 1; i >= 0; i--) {
			SubWindow sw = gui.sub_windows.write[i];

			// Clicked inside window?

			Rect2i r = Rect2i(sw.window->get_position(), sw.window->get_size());

			if (!sw.window->get_flag(Window::FLAG_BORDERLESS)) {
				// Check top bar.
				int title_height = sw.window->theme_cache.title_height;
				Rect2i title_bar = r;
				title_bar.position.y -= title_height;
				title_bar.size.y = title_height;

				if (title_bar.size.y > 0 && title_bar.has_point(mb_pos)) {
					click_on_window = sw.window;

					int close_h_ofs = sw.window->theme_cache.close_h_offset;
					int close_v_ofs = sw.window->theme_cache.close_v_offset;
					bool pressed = gui.subwindow_focused == sw.window &&
								   gui.subwindow_drag == SUB_WINDOW_DRAG_CLOSE &&
								   gui.subwindow_drag_close_inside;
					Ref<Texture2D> close_icon = pressed ? sw.window->theme_cache.close_pressed
														: sw.window->theme_cache.close;

					Rect2 close_rect;
					close_rect.position =
						Vector2(r.position.x + r.size.x - close_h_ofs, r.position.y - close_v_ofs);
					close_rect.size = close_icon->get_size();

					if (gui.subwindow_focused != sw.window) {
						// Refocus.
						_sub_window_grab_focus(sw.window);
					}

					if (close_rect.has_point(mb_pos)) {
						gui.subwindow_drag = SUB_WINDOW_DRAG_CLOSE;
						gui.subwindow_drag_close_inside = true; // Starts inside.
						gui.subwindow_drag_close_rect = close_rect;
					}
					else {
						gui.subwindow_drag = SUB_WINDOW_DRAG_MOVE;
					}

					gui.subwindow_drag_from = mb_pos;
					gui.subwindow_drag_pos = sw.window->get_position();

					_sub_window_update(sw.window);
				}
				else {
					gui.subwindow_resize_mode =
						_sub_window_get_resize_margin(sw.window, mb_pos);
					if (gui.subwindow_resize_mode != SUB_WINDOW_RESIZE_DISABLED) {
						if (gui.subwindow_focused != sw.window) {
							// Refocus.
							_sub_window_grab_focus(sw.window);
						}

						gui.subwindow_resize_from_rect = r;
						gui.subwindow_drag_from = mb_pos;
						gui.subwindow_drag = SUB_WINDOW_DRAG_RESIZE;
						click_on_window = sw.window;
					}
				}
			}
			if (!click_on_window && r.has_point(mb_pos)) {
				// Clicked, see if it needs to fetch focus.
				if (gui.subwindow_focused != sw.window) {
					// Refocus.
					_sub_window_grab_focus(sw.window);
				}

				click_on_window = sw.window;
			}

			if (click_on_window) {
				break;
			}
		}

		gui.currently_dragged_subwindow = click_on_window;

		if (!click_on_window && gui.subwindow_focused) {
			// No window found and clicked, remove focus.
			_sub_window_grab_focus(nullptr);
		}
	}

	if (gui.subwindow_focused && p_event.type == InputEventType::MOUSE_MOTION) {
		SubWindowResize resize = _sub_window_get_resize_margin(
			gui.subwindow_focused, p_event.mouse_motion.position);
		if (resize != SUB_WINDOW_RESIZE_DISABLED) {
			DisplayServerEnums::CursorShape shapes[SUB_WINDOW_RESIZE_MAX] = {
				DisplayServerEnums::CURSOR_ARROW, DisplayServerEnums::CURSOR_FDIAGSIZE,
				DisplayServerEnums::CURSOR_VSIZE, DisplayServerEnums::CURSOR_BDIAGSIZE,
				DisplayServerEnums::CURSOR_HSIZE, DisplayServerEnums::CURSOR_HSIZE,
				DisplayServerEnums::CURSOR_BDIAGSIZE, DisplayServerEnums::CURSOR_VSIZE,
				DisplayServerEnums::CURSOR_FDIAGSIZE};

			if (DisplayServer::has_feature(DisplayServerEnums::FEATURE_CURSOR_SHAPE)) {
				DisplayServer::cursor_set_shape(shapes[resize]);
			}

			return true; // Reserved for showing the resize cursor.
		}
	}

	if (gui.subwindow_drag != SUB_WINDOW_DRAG_DISABLED) {
		return true; // Dragging, don't pass the event.
	}

	if (!gui.subwindow_focused) {
		// No window focus, check for unfocusable windows under the cursor.
		if (p_event.is_mouse()) {
			const Vector2 me_pos = p_event.get_mouse_position();
			for (int i = gui.sub_windows.size() - 1; i >= 0; i--) {
				const SubWindow& sw = gui.sub_windows[i];
				if (!sw.window->get_flag(Window::FLAG_NO_FOCUS) ||
					sw.window->get_flag(Window::FLAG_MOUSE_PASSTHROUGH)) {
					continue;
				}
				Rect2i r = Rect2i(sw.window->get_position(), sw.window->get_size());
				if (r.has_point(me_pos)) {
					Transform2D window_ofs;
					window_ofs.set_origin(-sw.window->get_position());
					InputEvent ev = p_event.xformed_by(window_ofs);
					sw.window->_window_input(ev);
					return true;
				}
			}
		}
		return false;
	}

	Transform2D window_ofs;
	window_ofs.set_origin(-gui.subwindow_focused->get_position());

	InputEvent ev = p_event.xformed_by(window_ofs);

	gui.subwindow_focused->_window_input(ev);

	return true;
}

void Viewport::_window_start_drag(Window* p_window)
{
	int index = _sub_window_find(p_window);
	ERR_FAIL_COND(index == -1);

	SubWindow sw = gui.sub_windows.write[index];

	if (gui.subwindow_focused != sw.window) {
		// Refocus.
		_sub_window_grab_focus(sw.window);
	}

	gui.subwindow_drag = SUB_WINDOW_DRAG_MOVE;
	gui.subwindow_drag_from = get_mouse_position();
	gui.subwindow_drag_pos = sw.window->get_position();
	gui.currently_dragged_subwindow = sw.window;

	_sub_window_update(sw.window);
}

void Viewport::_window_start_resize(SubWindowResize p_edge, Window* p_window)
{
	int index = _sub_window_find(p_window);
	ERR_FAIL_COND(index == -1);

	SubWindow sw = gui.sub_windows.write[index];
	Rect2i r = Rect2i(sw.window->get_position(), sw.window->get_size());

	if (gui.subwindow_focused != sw.window) {
		// Refocus.
		_sub_window_grab_focus(sw.window);
	}

	gui.subwindow_drag = SUB_WINDOW_DRAG_RESIZE;
	gui.subwindow_resize_mode = p_edge;
	gui.subwindow_resize_from_rect = r;
	gui.subwindow_drag_from = get_mouse_position();
	gui.subwindow_drag_pos = sw.window->get_position();
	gui.currently_dragged_subwindow = sw.window;

	_sub_window_update(sw.window);
}

void Viewport::_update_mouse_over(const InputEvent& p_mm)
{
	if (!p_mm.is_mouse() || is_attached_in_viewport()) {
		return;
	}

	const Vector2 mm_pos = p_mm.get_mouse_position();
	if (SceneTree::get_root()->is_embedding_subwindows() || is_sub_viewport()) {
		_update_mouse_over(mm_pos);
	}
	else {
		Window* receiving_window = SceneTree::get_root()->gui.windowmanager_window_over;
		if (!receiving_window) {
			return;
		}
		if (receiving_window->get_window_id() != p_mm.window_id) {
			Vector2 pos = DisplayServer::mouse_get_position() - receiving_window->get_position();
			pos = receiving_window->get_final_transform().affine_inverse().xform(pos);
			receiving_window->_update_mouse_over(pos);
		}
		else {
			receiving_window->_update_mouse_over(mm_pos);
		}
	}
}

#ifndef DISABLE_DEPRECATED
void Viewport::push_unhandled_input(const InputEvent& p_event, bool p_local_coords)
{
	ERR_MAIN_THREAD_GUARD;
	WARN_DEPRECATED_MSG(
		R"*(The "push_unhandled_input()" method is deprecated, use "push_input()" instead.)*");
	ERR_FAIL_COND(!is_inside_tree());

	local_input_handled = false;

	if (disable_input || disable_input_override || !_can_consume_input_events()) {
		return;
	}

	if (Engine::is_editor_hint() && SceneTree::get_edited_scene_root() &&
		SceneTree::get_edited_scene_root()->is_ancestor_of(this)) {
		return;
	}

	InputEvent ev = p_local_coords ? p_event : _make_input_local(p_event);
	_push_unhandled_input_internal(ev);
}
#endif // DISABLE_DEPRECATED

void Viewport::notify_mouse_exited()
{
	if (!gui.mouse_in_viewport) {
		WARN_PRINT_ED("The Viewport was previously notified that the mouse has left its area. "
					  "There is no need to notify it at this time.");
		return;
	}
	_mouse_leave_viewport();
}

#if !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)
void Viewport::set_physics_object_picking(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	physics_object_picking = p_enable;
	if (physics_object_picking) {
		add_to_group("_picking_viewports");
	}
	else {
		physics_picking_events.clear();
		if (is_in_group("_picking_viewports")) {
			remove_from_group("_picking_viewports");
		}
	}
}

bool Viewport::get_physics_object_picking()
{
	ERR_READ_THREAD_GUARD_V(false);
	return physics_object_picking;
}

void Viewport::set_physics_object_picking_sort(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	physics_object_picking_sort = p_enable;
}

bool Viewport::get_physics_object_picking_sort()
{
	ERR_READ_THREAD_GUARD_V(false);
	return physics_object_picking_sort;
}

void Viewport::set_physics_object_picking_first_only(bool p_enable)
{
	physics_object_picking_first_only = p_enable;
}

bool Viewport::get_physics_object_picking_first_only() { return physics_object_picking_first_only; }
#endif // !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)

Vector2 Viewport::get_camera_coords(const Vector2& p_viewport_coords) const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	Transform2D xf = stretch_transform * global_canvas_transform;
	return xf.xform(p_viewport_coords);
}

Vector2 Viewport::get_camera_rect_size() const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	return size;
}

void Viewport::set_disable_input(bool p_disable)
{
	ERR_MAIN_THREAD_GUARD;
	if (p_disable == disable_input) {
		return;
	}
	if (p_disable && !disable_input_override) {
		_drop_mouse_focus();
		_mouse_leave_viewport();
		_gui_cancel_tooltip();
	}
	disable_input = p_disable;
}

bool Viewport::is_input_disabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return disable_input;
}

void Viewport::set_disable_input_override(bool p_disable)
{
	ERR_MAIN_THREAD_GUARD;
	if (p_disable == disable_input_override) {
		return;
	}
	if (p_disable && !disable_input) {
		_drop_mouse_focus();
		_mouse_leave_viewport();
		_gui_cancel_tooltip();
	}
	disable_input_override = p_disable;
}

String Viewport::gui_get_drag_description() const
{
	ERR_READ_THREAD_GUARD_V(String());
	if (get_section_root_viewport()->gui.drag_description.is_empty()) {
		return RTR("Drag-and-drop data");
	}
	else {
		return get_section_root_viewport()->gui.drag_description;
	}
}

void Viewport::gui_set_drag_description(const String& p_description)
{
	gui.drag_description = p_description;
}

PackedStringArray Viewport::get_configuration_warnings() const
{
	ERR_MAIN_THREAD_GUARD_V(PackedStringArray());
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (size.x <= 1 || size.y <= 1) {
		warnings.push_back(RTR("The Viewport size must be greater than or equal to 2 pixels on "
							   "both dimensions to render anything."));
	}
	return warnings;
}

void Viewport::gui_reset_canvas_sort_index()
{
	ERR_MAIN_THREAD_GUARD;
	gui.canvas_sort_index = 0;
}

int Viewport::gui_get_canvas_sort_index()
{
	ERR_MAIN_THREAD_GUARD_V(0);
	return gui.canvas_sort_index++;
}

Control* Viewport::gui_get_focus_owner() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return gui.key_focus;
}

void Viewport::set_msaa_2d(MSAA p_msaa)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_msaa, MSAA_MAX);
	if (msaa_2d == p_msaa) {
		return;
	}
	msaa_2d = p_msaa;
	Renderer::viewport_set_msaa_2d(viewport, RSE::ViewportMSAA(p_msaa));
}

Viewport::MSAA Viewport::get_msaa_2d() const
{
	ERR_READ_THREAD_GUARD_V(MSAA_DISABLED);
	return msaa_2d;
}

void Viewport::set_msaa_3d(MSAA p_msaa)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_msaa, MSAA_MAX);
	if (msaa_3d == p_msaa) {
		return;
	}
	msaa_3d = p_msaa;
	Renderer::viewport_set_msaa_3d(viewport, RSE::ViewportMSAA(p_msaa));
}

Viewport::MSAA Viewport::get_msaa_3d() const
{
	ERR_READ_THREAD_GUARD_V(MSAA_DISABLED);
	return msaa_3d;
}

void Viewport::set_screen_space_aa(ScreenSpaceAA p_screen_space_aa)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_screen_space_aa, SCREEN_SPACE_AA_MAX);
	if (screen_space_aa == p_screen_space_aa) {
		return;
	}
	screen_space_aa = p_screen_space_aa;
	Renderer::viewport_set_screen_space_aa(viewport, RSE::ViewportScreenSpaceAA(p_screen_space_aa));
}

Viewport::ScreenSpaceAA Viewport::get_screen_space_aa() const
{
	ERR_READ_THREAD_GUARD_V(SCREEN_SPACE_AA_DISABLED);
	return screen_space_aa;
}

void Viewport::set_use_taa(bool p_use_taa)
{
	ERR_MAIN_THREAD_GUARD;
	if (use_taa == p_use_taa) {
		return;
	}
	use_taa = p_use_taa;
	Renderer::viewport_set_use_taa(viewport, p_use_taa);
}

bool Viewport::is_using_taa() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_taa;
}

void Viewport::set_use_debanding(bool p_use_debanding)
{
	ERR_MAIN_THREAD_GUARD;
	if (use_debanding == p_use_debanding) {
		return;
	}
	use_debanding = p_use_debanding;
	Renderer::viewport_set_use_debanding(viewport, p_use_debanding);
}

bool Viewport::is_using_debanding() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_debanding;
}

void Viewport::set_mesh_lod_threshold(float p_pixels)
{
	ERR_MAIN_THREAD_GUARD;
	mesh_lod_threshold = p_pixels;
	Renderer::viewport_set_mesh_lod_threshold(viewport, mesh_lod_threshold);
}

float Viewport::get_mesh_lod_threshold() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return mesh_lod_threshold;
}

bool Viewport::is_using_occlusion_culling() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_occlusion_culling;
}

void Viewport::set_debug_draw(DebugDraw p_debug_draw)
{
	ERR_MAIN_THREAD_GUARD;
	debug_draw = p_debug_draw;
	Renderer::viewport_set_debug_draw(viewport, RSE::ViewportDebugDraw(p_debug_draw));
}

Viewport::DebugDraw Viewport::get_debug_draw() const
{
	ERR_READ_THREAD_GUARD_V(DEBUG_DRAW_DISABLED);
	return debug_draw;
}

int Viewport::get_render_info(RenderInfoType p_type, RenderInfo p_info)
{
	ERR_READ_THREAD_GUARD_V(0);
	return Renderer::viewport_get_render_info(
		viewport, RSE::ViewportRenderInfoType(p_type), RSE::ViewportRenderInfo(p_info));
}

void Viewport::set_snap_controls_to_pixels(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	snap_controls_to_pixels = p_enable;
}

bool Viewport::is_snap_controls_to_pixels_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return snap_controls_to_pixels;
}

void Viewport::set_snap_2d_transforms_to_pixel(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	snap_2d_transforms_to_pixel = p_enable;
	Renderer::viewport_set_snap_2d_transforms_to_pixel(viewport, snap_2d_transforms_to_pixel);
}

bool Viewport::is_snap_2d_transforms_to_pixel_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return snap_2d_transforms_to_pixel;
}

void Viewport::set_snap_2d_vertices_to_pixel(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	snap_2d_vertices_to_pixel = p_enable;
	Renderer::viewport_set_snap_2d_vertices_to_pixel(viewport, snap_2d_vertices_to_pixel);
}

bool Viewport::is_snap_2d_vertices_to_pixel_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return snap_2d_vertices_to_pixel;
}

bool Viewport::gui_is_dragging() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return get_section_root_viewport()->gui.global_dragging;
}

bool Viewport::gui_is_drag_successful() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return gui.drag_successful;
}

void Viewport::gui_cancel_drag()
{
	ERR_MAIN_THREAD_GUARD;
	if (gui_is_dragging()) {
		_perform_drop();
	}
}

void Viewport::set_handle_input_locally(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	handle_input_locally = p_enable;
}

bool Viewport::is_handling_input_locally() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return handle_input_locally;
}

void Viewport::set_propagate_shortcuts_to_parent(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	propagate_shortcuts_to_parent = p_enable;
}

bool Viewport::gui_shortcut_use_focus_owner() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return shortcut_use_focus_owner;
}

void Viewport::set_default_canvas_item_texture_filter(DefaultCanvasItemTextureFilter p_filter)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_filter, DEFAULT_CANVAS_ITEM_TEXTURE_FILTER_MAX);

	if (default_canvas_item_texture_filter == p_filter) {
		return;
	}
	default_canvas_item_texture_filter = p_filter;
	_update_texture_filter_changed(true);
}

Viewport::DefaultCanvasItemTextureFilter Viewport::get_default_canvas_item_texture_filter() const
{
	ERR_READ_THREAD_GUARD_V(DEFAULT_CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
	return default_canvas_item_texture_filter;
}

RenderingServerEnums::CanvasItemTextureFilter Viewport::get_texture_filter_in_tree() const
{
	ERR_READ_THREAD_GUARD_V(RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR);
	_refresh_texture_filter_cache();
	return default_canvas_item_texture_filter_cache;
}

void Viewport::set_default_canvas_item_texture_repeat(DefaultCanvasItemTextureRepeat p_repeat)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_repeat, DEFAULT_CANVAS_ITEM_TEXTURE_REPEAT_MAX);

	if (default_canvas_item_texture_repeat == p_repeat) {
		return;
	}

	default_canvas_item_texture_repeat = p_repeat;
	_update_texture_repeat_changed(true);
}

Viewport::DefaultCanvasItemTextureRepeat Viewport::get_default_canvas_item_texture_repeat() const
{
	ERR_READ_THREAD_GUARD_V(DEFAULT_CANVAS_ITEM_TEXTURE_REPEAT_DISABLED);
	return default_canvas_item_texture_repeat;
}

RenderingServerEnums::CanvasItemTextureRepeat Viewport::get_texture_repeat_in_tree() const
{
	ERR_READ_THREAD_GUARD_V(RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED);
	_refresh_texture_repeat_cache();
	return default_canvas_item_texture_repeat_cache;
}

Viewport::VRSMode Viewport::get_vrs_mode() const
{
	ERR_READ_THREAD_GUARD_V(VRS_DISABLED);
	return vrs_mode;
}

void Viewport::set_vrs_update_mode(VRSUpdateMode p_vrs_update_mode)
{
	ERR_MAIN_THREAD_GUARD;

	vrs_update_mode = p_vrs_update_mode;
	switch (p_vrs_update_mode) {
	case VRS_UPDATE_ONCE: {
		Renderer::viewport_set_vrs_update_mode(viewport, RSE::VIEWPORT_VRS_UPDATE_ONCE);
	} break;
	case VRS_UPDATE_ALWAYS: {
		Renderer::viewport_set_vrs_update_mode(viewport, RSE::VIEWPORT_VRS_UPDATE_ALWAYS);
	} break;
	default: {
		Renderer::viewport_set_vrs_update_mode(viewport, RSE::VIEWPORT_VRS_UPDATE_DISABLED);
	} break;
	}
}

Viewport::VRSUpdateMode Viewport::get_vrs_update_mode() const
{
	ERR_READ_THREAD_GUARD_V(VRS_UPDATE_DISABLED);
	return vrs_update_mode;
}

void Viewport::set_vrs_texture(Ref<Texture2D> p_texture)
{
	ERR_MAIN_THREAD_GUARD;
	vrs_texture = p_texture;

	// TODO need to add something here in case the RID changes
	RID tex = p_texture.is_valid() ? p_texture->get_rid() : RID();
	Renderer::viewport_set_vrs_texture(viewport, tex);
}

Ref<Texture2D> Viewport::get_vrs_texture() const
{
	ERR_READ_THREAD_GUARD_V(Ref<Texture2D>());
	return vrs_texture;
}

DisplayServerEnums::WindowID Viewport::get_window_id() const
{
	ERR_READ_THREAD_GUARD_V(DisplayServerEnums::INVALID_WINDOW_ID);
	return DisplayServerEnums::MAIN_WINDOW_ID;
}

Viewport* Viewport::get_parent_viewport() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	ERR_FAIL_COND_V(!is_inside_tree(), nullptr);
	if (!get_parent()) {
		return nullptr; // root viewport
	}

	return get_parent()->get_viewport();
}

void Viewport::set_embedding_subwindows(bool p_embed)
{
	ERR_THREAD_GUARD;
	if (gui.embed_subwindows_hint == p_embed) {
		return;
	}

	bool allow_change = true;

	if (!is_inside_tree()) {
		// Change can happen since no child window is displayed.
	}
	else if (gui.embed_subwindows_hint) {
		if (!gui.sub_windows.is_empty()) {
			// Prevent change when this viewport has embedded windows.
			allow_change = false;
		}
	}
	else {
		Viewport* vp = this;
		while (true) {
			if (!vp->get_parent()) {
				// Root window reached.
				break;
			}
			vp = vp->get_parent()->get_viewport();
			if (vp->is_embedding_subwindows()) {
				for (int i = 0; i < vp->gui.sub_windows.size(); i++) {
					if (is_ancestor_of(vp->gui.sub_windows[i].window)) {
						// Prevent change when this viewport has child windows that are displayed in
						// an ancestor viewport.
						allow_change = false;
						break;
					}
				}
			}
		}

		if (allow_change) {
			Vector<DisplayServerEnums::WindowID> wl =
				DisplayServer::get_window_list();
			for (const DisplayServerEnums::WindowID& window_id : wl) {
				const Window* w = Window::get_from_id(window_id);
				if (w && is_ancestor_of(w)) {
					// Prevent change when this viewport has child windows that are displayed as
					// native windows.
					allow_change = false;
					break;
				}
			}
		}
	}

	if (allow_change) {
		gui.embed_subwindows_hint = p_embed;
	}
	else {
		WARN_PRINT("Can't change \"gui_embed_subwindows\" while a child window is displayed. "
				   "Consider hiding all child windows before changing this value.");
	}
}

bool Viewport::is_embedding_subwindows() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return gui.embed_subwindows_hint;
}

void Viewport::set_drag_threshold(int p_threshold)
{
	ERR_MAIN_THREAD_GUARD
	gui.drag_threshold = p_threshold;
}

int Viewport::get_drag_threshold() const
{
	ERR_READ_THREAD_GUARD_V(10);
	return gui.drag_threshold;
}

void Viewport::subwindow_set_popup_safe_rect(Window* p_window, const Rect2i& p_rect)
{
	int index = _sub_window_find(p_window);
	ERR_FAIL_COND(index == -1);

	gui.sub_windows.write[index].parent_safe_rect = p_rect;
}

Rect2i Viewport::subwindow_get_popup_safe_rect(Window* p_window) const
{
	int index = _sub_window_find(p_window);
	// FIXME: Re-enable ERR_FAIL_COND after rewriting embedded window popup closing.
	// Currently it is expected, that index == -1 can happen.
	if (index == -1) {
		return Rect2i();
	}
	// ERR_FAIL_COND_V(index == -1, Rect2i());

	return gui.sub_windows[index].parent_safe_rect;
}

void Viewport::set_sdf_oversize(SDFOversize p_sdf_oversize)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_sdf_oversize, SDF_OVERSIZE_MAX);
	sdf_oversize = p_sdf_oversize;
	Renderer::viewport_set_sdf_oversize_and_scale(
		viewport, RSE::ViewportSDFOversize(sdf_oversize), RSE::ViewportSDFScale(sdf_scale));
}

Viewport::SDFOversize Viewport::get_sdf_oversize() const
{
	ERR_READ_THREAD_GUARD_V(SDF_OVERSIZE_100_PERCENT);
	return sdf_oversize;
}

void Viewport::set_sdf_scale(SDFScale p_sdf_scale)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_sdf_scale, SDF_SCALE_MAX);
	sdf_scale = p_sdf_scale;
	Renderer::viewport_set_sdf_oversize_and_scale(
		viewport, RSE::ViewportSDFOversize(sdf_oversize), RSE::ViewportSDFScale(sdf_scale));
}

Viewport::SDFScale Viewport::get_sdf_scale() const
{
	ERR_READ_THREAD_GUARD_V(SDF_SCALE_100_PERCENT);
	return sdf_scale;
}

Transform2D Viewport::get_screen_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return get_screen_transform_internal();
}

Transform2D Viewport::get_screen_transform_internal(bool p_absolute_position) const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return get_final_transform();
}

void Viewport::update_mouse_cursor_state()
{
	// Updates need to happen in Window, because SubViewportContainers might be hidden behind other
	// Controls.
	Window* base_window = get_base_window();
	if (base_window) {
		base_window->update_mouse_cursor_state();
	}
}

template <> Camera3D* Viewport::CameraOverride<Camera3D>::get_overridden_camera() const {}

template <> Camera2D* Viewport::CameraOverride<Camera2D>::get_overridden_camera() const {}

template <> void Viewport::CameraOverride<Camera3D>::enable(Viewport*, Camera3D const*) {}

template <> void Viewport::CameraOverride<Camera3D>::disable(Camera3D*) {}

template <> void Viewport::CameraOverride<Camera2D>::set_overridden_camera(Camera2D const*) {}

template <> void Viewport::CameraOverride<Camera2D>::enable(Viewport*, Camera2D const*) {}

template <> void Viewport::CameraOverride<Camera2D>::disable(Camera2D*) {}

void Viewport::set_canvas_cull_mask(uint32_t p_canvas_cull_mask)
{
	ERR_MAIN_THREAD_GUARD;
	canvas_cull_mask = p_canvas_cull_mask;
	Renderer::viewport_set_canvas_cull_mask(viewport, canvas_cull_mask);
}

uint32_t Viewport::get_canvas_cull_mask() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return canvas_cull_mask;
}

void Viewport::set_canvas_cull_mask_bit(uint32_t p_layer, bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_UNSIGNED_INDEX(p_layer, 32);
	if (p_enable) {
		set_canvas_cull_mask(canvas_cull_mask | (1 << p_layer));
	}
	else {
		set_canvas_cull_mask(canvas_cull_mask & (~(1 << p_layer)));
	}
}

bool Viewport::get_canvas_cull_mask_bit(uint32_t p_layer) const
{
	ERR_READ_THREAD_GUARD_V(false);
	ERR_FAIL_UNSIGNED_INDEX_V(p_layer, 32, false);
	return (canvas_cull_mask & (1 << p_layer));
}

void Viewport::_update_audio_listener_2d()
{
	if (AudioServer::is_initialized()) {
		AudioServer::notify_listener_changed();
	}
}

void Viewport::_audio_listener_2d_set(AudioListener2D* p_audio_listener)
{
	if (audio_listener_2d == p_audio_listener) {
		return;
	}
	else if (audio_listener_2d) {
		audio_listener_2d->clear_current();
	}
	audio_listener_2d = p_audio_listener;
}

void Viewport::_audio_listener_2d_remove(AudioListener2D* p_audio_listener)
{
	if (audio_listener_2d == p_audio_listener) {
		audio_listener_2d = nullptr;
	}
}

void Viewport::_camera_2d_set(Camera2D* p_camera_2d)
{
#if DEBUG_ENABLED
	if (is_camera_2d_override_enabled()) {
		camera_2d_override.set_overridden_camera(p_camera_2d);
		return;
	}
#endif // DEBUG_ENABLED

	camera_2d = p_camera_2d;
}

AudioListener2D* Viewport::get_audio_listener_2d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return audio_listener_2d;
}

void Viewport::set_as_audio_listener_2d(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	if (p_enable == is_audio_listener_2d_enabled) {
		return;
	}

	is_audio_listener_2d_enabled = p_enable;
	_update_audio_listener_2d();
}

bool Viewport::is_audio_listener_2d() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return is_audio_listener_2d_enabled;
}

Camera2D* Viewport::get_camera_2d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return camera_2d;
}

#if DEBUG_ENABLED
void Viewport::enable_camera_2d_override(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;

	if (p_enable) {
		camera_2d_override.enable(this, camera_2d);
	}
	else {
		camera_2d_override.disable(camera_2d);
	}
}

bool Viewport::is_camera_2d_override_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return camera_2d_override.is_enabled();
}

Camera2D* Viewport::get_overridden_camera_2d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	ERR_FAIL_COND_V(!camera_2d_override.is_enabled(), nullptr);
	return camera_2d_override.get_overridden_camera();
}

Camera2D* Viewport::get_override_camera_2d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	ERR_FAIL_COND_V(!camera_2d_override.is_enabled(), nullptr);
	return camera_2d_override.is_enabled() ? get_camera_2d() : nullptr;
}
#endif // DEBUG_ENABLED

#ifndef _3D_DISABLED
AudioListener3D* Viewport::get_audio_listener_3d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return audio_listener_3d;
}

void Viewport::set_as_audio_listener_3d(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	if (p_enable == is_audio_listener_3d_enabled) {
		return;
	}

	is_audio_listener_3d_enabled = p_enable;
	_update_audio_listener_3d();
}

bool Viewport::is_audio_listener_3d() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return is_audio_listener_3d_enabled;
}

void Viewport::_update_audio_listener_3d()
{
	if (AudioServer::is_initialized()) {
		AudioServer::notify_listener_changed();
	}
}

void Viewport::_listener_transform_3d_changed_notify() {}

void Viewport::_audio_listener_3d_set(AudioListener3D* p_listener)
{
	if (audio_listener_3d == p_listener) {
		return;
	}

	audio_listener_3d = p_listener;

	_update_audio_listener_3d();
	_listener_transform_3d_changed_notify();
}

bool Viewport::_audio_listener_3d_add(AudioListener3D* p_listener)
{
	audio_listener_3d_set.insert(p_listener);
	return audio_listener_3d_set.size() == 1;
}

void Viewport::_audio_listener_3d_remove(AudioListener3D* p_listener)
{
	audio_listener_3d_set.erase(p_listener);
	if (audio_listener_3d == p_listener) {
		audio_listener_3d = nullptr;
	}
}

void Viewport::_audio_listener_3d_make_next_current(AudioListener3D* p_exclude)
{
	if (audio_listener_3d_set.size() > 0) {
		for (AudioListener3D* E : audio_listener_3d_set) {
			if (p_exclude == E) {
				continue;
			}
			if (!E->is_inside_tree()) {
				continue;
			}
			if (audio_listener_3d != nullptr) {
				return;
			}

			E->make_current();
		}
	}
	else {
		// Attempt to reset listener to the camera position.
		if (camera_3d != nullptr) {
			_update_audio_listener_3d();
			_camera_3d_transform_changed_notify();
		}
	}
}

Camera3D* Viewport::get_camera_3d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return camera_3d;
}

void Viewport::_camera_3d_transform_changed_notify() {}

bool Viewport::_camera_3d_add(Camera3D* p_camera)
{
	camera_3d_set.insert(p_camera);
	return camera_3d_set.size() == 1;
}

void Viewport::_camera_3d_remove(Camera3D* p_camera)
{
	camera_3d_set.erase(p_camera);
	if (camera_3d == p_camera) {
		_camera_3d_set(nullptr);
	}
}

void Viewport::_camera_3d_make_next_current(Camera3D* p_exclude)
{
	for (Camera3D* E : camera_3d_set) {
		if (p_exclude == E) {
			continue;
		}
		if (!E->is_inside_tree()) {
			continue;
		}
		if (camera_3d != nullptr) {
			return;
		}

		E->make_current();
	}
}

#if DEBUG_ENABLED
void Viewport::enable_camera_3d_override(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;

	if (p_enable) {
		camera_3d_override.enable(this, camera_3d);
	}
	else {
		camera_3d_override.disable(camera_3d);
	}
}

bool Viewport::is_camera_3d_override_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return camera_3d_override.is_enabled();
}

Camera3D* Viewport::get_overridden_camera_3d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	ERR_FAIL_COND_V(!camera_3d_override.is_enabled(), nullptr);
	return camera_3d_override.get_overridden_camera();
}

Camera3D* Viewport::get_override_camera_3d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	ERR_FAIL_COND_V(!camera_3d_override.is_enabled(), nullptr);
	return get_camera_3d();
}
#endif // DEBUG_ENABLED

void Viewport::set_disable_3d(bool p_disable)
{
	ERR_MAIN_THREAD_GUARD;
	disable_3d = p_disable;
	Renderer::viewport_set_disable_3d(viewport, disable_3d);
}

bool Viewport::is_3d_disabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return disable_3d;
}

Ref<World3D> Viewport::get_world_3d() const
{
	ERR_READ_THREAD_GUARD_V(Ref<World3D>());
	return world_3d;
}

Ref<World3D> Viewport::find_world_3d() const
{
	ERR_READ_THREAD_GUARD_V(Ref<World3D>());
	if (own_world_3d.is_valid()) {
		return own_world_3d;
	}
	else if (world_3d.is_valid()) {
		return world_3d;
	}
	else if (parent) {
		return parent->find_world_3d();
	}
	else {
		return Ref<World3D>();
	}
}

void Viewport::_own_world_3d_changed()
{
	ERR_FAIL_COND(world_3d.is_null());
	ERR_FAIL_COND(own_world_3d.is_null());

	if (is_inside_tree()) {
		_propagate_exit_world_3d(this);
	}

	if (is_inside_tree()) {
		_propagate_enter_world_3d(this);
	}

	if (is_inside_tree()) {
		Renderer::viewport_set_scenario(viewport, find_world_3d()->get_scenario());
	}

	_update_audio_listener_3d();
}

bool Viewport::is_using_own_world_3d() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return own_world_3d.is_valid();
}

#ifndef XR_DISABLED

bool Viewport::is_using_xr() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_xr;
}
#endif // XR_DISABLED

void Viewport::set_scaling_3d_mode(Scaling3DMode p_scaling_3d_mode)
{
	ERR_MAIN_THREAD_GUARD;
	if (scaling_3d_mode == p_scaling_3d_mode) {
		return;
	}

	scaling_3d_mode = p_scaling_3d_mode;
	Renderer::viewport_set_scaling_3d_mode(viewport, (RSE::ViewportScaling3DMode)(int)p_scaling_3d_mode);
}

Viewport::Scaling3DMode Viewport::get_scaling_3d_mode() const
{
	ERR_READ_THREAD_GUARD_V(SCALING_3D_MODE_BILINEAR);
	return scaling_3d_mode;
}

void Viewport::set_scaling_3d_scale(float p_scaling_3d_scale)
{
	ERR_MAIN_THREAD_GUARD;
	// Clamp to reasonable values that are actually useful.
	// Values above 2.0 don't serve a practical purpose since the viewport
	// isn't displayed with mipmaps.
	scaling_3d_scale = CLAMP(p_scaling_3d_scale, 0.1, 2.0);

	Renderer::viewport_set_scaling_3d_scale(viewport, scaling_3d_scale);
}

float Viewport::get_scaling_3d_scale() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return scaling_3d_scale;
}

void Viewport::set_fsr_sharpness(float p_fsr_sharpness)
{
	ERR_MAIN_THREAD_GUARD;
	if (fsr_sharpness == p_fsr_sharpness) {
		return;
	}

	if (p_fsr_sharpness < 0.0f) {
		p_fsr_sharpness = 0.0f;
	}

	fsr_sharpness = p_fsr_sharpness;
	Renderer::viewport_set_fsr_sharpness(viewport, p_fsr_sharpness);
}

float Viewport::get_fsr_sharpness() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return fsr_sharpness;
}

void Viewport::set_texture_mipmap_bias(float p_texture_mipmap_bias)
{
	ERR_MAIN_THREAD_GUARD;
	if (texture_mipmap_bias == p_texture_mipmap_bias) {
		return;
	}

	texture_mipmap_bias = p_texture_mipmap_bias;
	Renderer::viewport_set_texture_mipmap_bias(viewport, p_texture_mipmap_bias);
}

float Viewport::get_texture_mipmap_bias() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return texture_mipmap_bias;
}

void Viewport::set_anisotropic_filtering_level(AnisotropicFiltering p_anisotropic_filtering_level)
{
	ERR_MAIN_THREAD_GUARD;
	if (anisotropic_filtering_level == p_anisotropic_filtering_level) {
		return;
	}

	anisotropic_filtering_level = p_anisotropic_filtering_level;
	Renderer::viewport_set_anisotropic_filtering_level(
		viewport, (RSE::ViewportAnisotropicFiltering)(int)p_anisotropic_filtering_level);
}

Viewport::AnisotropicFiltering Viewport::get_anisotropic_filtering_level() const
{
	ERR_READ_THREAD_GUARD_V(ANISOTROPY_DISABLED);
	return anisotropic_filtering_level;
}

#endif // _3D_DISABLED

Viewport::~Viewport()
{
	// Erase itself from viewport textures.
	for (ViewportTexture* E : viewport_textures) {
		E->vp = nullptr;
	}
	if (world_2d.is_valid()) {
		world_2d->remove_viewport(this);
	}
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(viewport);
}

void SubViewport::set_size(const Size2i& p_size)
{
	ERR_MAIN_THREAD_GUARD;
	_internal_set_size(p_size, _get_view_count());
}

void SubViewport::set_size_force(const Size2i& p_size)
{
	ERR_MAIN_THREAD_GUARD;
	// Use only for setting the size from the parent SubViewportContainer with enabled stretch mode.
	// Don't expose function to scripting.
	_internal_set_size(p_size, _get_view_count(), true);
}

Size2i SubViewport::get_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	return _get_size();
}

void SubViewport::set_view_count(const int p_view_count)
{
	ERR_MAIN_THREAD_GUARD;

	// Q: add `, _get_size_2d_override(), true` ?
	_internal_set_size(_get_size(), p_view_count);
}

int SubViewport::get_view_count() const
{
	ERR_READ_THREAD_GUARD_V(1);

	return _get_view_count();
}

void SubViewport::set_size_2d_override(const Size2i& p_size)
{
	ERR_MAIN_THREAD_GUARD;
	_set_size(_get_size(), _get_view_count(), p_size, true);
}

Size2i SubViewport::get_size_2d_override() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	// Rounding will cause offset issues with the
	// exact positioning of subwindows, but changing the
	// type of size_2d_override would break compatibility.
	return Size2i((_get_size_2d_override() + Size2(0.5, 0.5)).floor());
}

void SubViewport::set_size_2d_override_stretch(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	if (p_enable == size_2d_override_stretch) {
		return;
	}

	size_2d_override_stretch = p_enable;
	_set_size(_get_size(), _get_view_count(), _get_size_2d_override(), true);
}

bool SubViewport::is_size_2d_override_stretch_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return size_2d_override_stretch;
}

void SubViewport::set_update_mode(UpdateMode p_mode)
{
	ERR_MAIN_THREAD_GUARD;
	update_mode = p_mode;
	Renderer::viewport_set_update_mode(get_viewport_rid(), RSE::ViewportUpdateMode(p_mode));
}

SubViewport::UpdateMode SubViewport::get_update_mode() const
{
	ERR_READ_THREAD_GUARD_V(UPDATE_DISABLED);
	return update_mode;
}

void SubViewport::set_clear_mode(ClearMode p_mode)
{
	ERR_MAIN_THREAD_GUARD;
	clear_mode = p_mode;
	Renderer::viewport_set_clear_mode(get_viewport_rid(), RSE::ViewportClearMode(p_mode));
}

SubViewport::ClearMode SubViewport::get_clear_mode() const
{
	ERR_READ_THREAD_GUARD_V(CLEAR_MODE_ALWAYS);
	return clear_mode;
}

DisplayServerEnums::WindowID SubViewport::get_window_id() const
{
	ERR_READ_THREAD_GUARD_V(DisplayServerEnums::INVALID_WINDOW_ID);
	return DisplayServerEnums::INVALID_WINDOW_ID;
}

SubViewport::SubViewport()
{
	Renderer::viewport_set_size(
		get_viewport_rid(), get_size().width, get_size().height, get_view_count());
}

#if DEBUG_ENABLED
template <class T> bool Viewport::CameraOverride<T>::is_enabled() const { return enabled; }

// Explicit template instantiation to allow template definitions inside cpp file
// and prevent instantiation using other than the desired camera types.
template class Viewport::CameraOverride<Camera2D>;
#ifndef _3D_DISABLED
template class Viewport::CameraOverride<Camera3D>;
#endif // _3D_DISABLED
#endif // DEBUG_ENABLED

void Viewport::_sub_window_grab_focus(Window* p_window) {}

bool Viewport::_set_size(
	const Size2i& p_size, const int p_view_count, const Size2& p_size_2d_override, bool p_allocated)
{
	size = p_size;
	Renderer::viewport_set_size(viewport, p_size.width, p_size.height, p_view_count);
	return true;
}

void Viewport::set_input_as_handled() {}

bool Viewport::is_input_handled() const { return false; }

void Viewport::_camera_3d_set(Camera3D* p_camera) {}

bool Viewport::is_visible_subviewport() const { return false; }

void Viewport::gui_perform_drop_at(Vector2 const&, Control*) {}

Window* Viewport::get_base_window() {}

void SubViewport::_internal_set_size(Vector2i const&, int, bool) {}

Viewport::Viewport()
{
	viewport = Renderer::viewport_create();
	world_2d.instantiate();
	world_2d->register_viewport(this);
	current_canvas = world_2d->get_canvas();
	Renderer::viewport_attach_canvas(viewport, current_canvas);
}

void Viewport::_push_text_input(String const&, bool) {}

void Viewport::_gui_update_mouse_over() {}

void Viewport::_gui_show_tooltip_at(Vector2i const&) {}

void Viewport::gui_release_focus() {}

void Viewport::_drop_mouse_focus() {}

void Viewport::_update_texture_repeat_changed(bool) {}

void Viewport::_update_texture_filter_changed(bool) {}

void Viewport::_refresh_texture_repeat_cache() const {}

void Viewport::_refresh_texture_filter_cache() const {}

void Viewport::_push_unhandled_input_internal(const InputEvent&) {}

void Viewport::push_input(const InputEvent&, bool) {}

void Viewport::_propagate_world_2d_changed(Node*) {}

void Viewport::_propagate_exit_world_3d(Node*) {}

void Viewport::_propagate_enter_world_3d(Node*) {}

void Viewport::_mouse_leave_viewport() {}

void Viewport::_update_mouse_over(Vector2 p_pos) {}

InputEvent Viewport::_make_input_local(const InputEvent& p_ev)
{
	return p_ev;
}

void Viewport::_gui_sort_roots() {}

void Viewport::_gui_set_drag_preview(Control*, Control*) {}

void Viewport::_gui_grab_click_focus(Control*) {}

Control* Viewport::_gui_find_control_at_pos(CanvasItem*, Vector2 const&, Transform2D const&)
{
	Control c = Control();
	return &c;
}

void Viewport::_gui_control_grab_focus(Control*, bool) {}

void ViewportTexture::setup_local_to_scene() {}

Transform2D SubViewport::get_screen_transform_internal(bool p_include_canvas_transform) const
{
	return Transform2D();
}

Transform2D SubViewport::get_popup_base_transform() const { return Transform2D(); }

Viewport* SubViewport::get_section_root_viewport() const { return nullptr; }

bool SubViewport::is_attached_in_viewport() const { return false; }

// --- viewport.compat.inc ---

#ifndef DISABLE_DEPRECATED

#include "viewport.h"

#ifndef XR_DISABLED
bool Viewport::_is_using_xr_115799() { return is_using_xr(); }
#endif

void Viewport::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED



// ============================================================================
// Merged from scene/main/window.cpp
// ============================================================================

#include "window.h"

STATIC_ASSERT_INCOMPLETE_TYPE(class, Renderer);

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/os/os.h"
#include "scene/gui/control.h"
#include "scene/main/scene_tree.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/rendering_server_enums.h"

// Editor integration.

int Window::root_layout_direction = 0;

void Window::set_root_layout_direction(int p_root_dir) { root_layout_direction = p_root_dir; }

// Dynamic properties.

Window* Window::focused_window = nullptr;

void Window::set_title(const String& p_title) {}

String Window::get_title() const
{
	ERR_READ_THREAD_GUARD_V(String());
	return title;
}

String Window::get_displayed_title() const
{
	ERR_READ_THREAD_GUARD_V(String());
	return displayed_title;
}

Window::WindowInitialPosition Window::get_initial_position() const
{
	ERR_READ_THREAD_GUARD_V(WINDOW_INITIAL_POSITION_ABSOLUTE);
	return initial_position;
}

void Window::set_current_screen(int p_screen)
{
	ERR_MAIN_THREAD_GUARD;

	current_screen = p_screen;
	if (window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}
	DisplayServer::window_set_current_screen(p_screen, window_id);
}

int Window::get_current_screen() const
{
	ERR_READ_THREAD_GUARD_V(0);

	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		current_screen = DisplayServer::window_get_current_screen(window_id);
	}
	return current_screen;
}

void Window::set_position(const Point2i& p_position)
{
	ERR_MAIN_THREAD_GUARD;

	position = p_position;

	if (embedder) {
		embedder->_sub_window_update(this);

	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_position(p_position, window_id);
	}
}

Point2i Window::get_position() const
{
	ERR_READ_THREAD_GUARD_V(Point2i());

	return position;
}

void Window::move_to_center()
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!is_inside_tree());

	Rect2 parent_rect;

	if (is_embedded()) {
		parent_rect = get_embedder()->get_visible_rect();
	}
	else {
		int parent_screen =
			DisplayServer::window_get_current_screen(get_window_id());
		parent_rect.position = DisplayServer::screen_get_position(parent_screen);
		parent_rect.size = DisplayServer::screen_get_size(parent_screen);
	}

	if (parent_rect != Rect2()) {
		set_position(parent_rect.position + (parent_rect.size - get_size()) / 2);
	}
}

void Window::set_size(const Size2i& p_size)
{
	ERR_MAIN_THREAD_GUARD;
#if defined(ANDROID_ENABLED)
	if (!get_parent() && is_inside_tree()) {
		// Can't set root window size on Android.
		return;
	}
#endif

	size = p_size;
	_update_window_size();
	_settings_changed();
}

Size2i Window::get_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	return size;
}

void Window::reset_size()
{
	ERR_MAIN_THREAD_GUARD;
	set_size(Size2i());
}

Point2i Window::get_position_with_decorations() const
{
	ERR_READ_THREAD_GUARD_V(Point2i());
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_get_position_with_decorations(window_id);
	}
	if (visible && is_embedded() && !get_flag(Window::FLAG_BORDERLESS)) {
		Size2 border_offset;
		if (theme_cache.embedded_border.is_valid()) {
			border_offset = theme_cache.embedded_border->get_offset();
		}
		if (theme_cache.embedded_unfocused_border.is_valid()) {
			border_offset = border_offset.max(theme_cache.embedded_unfocused_border->get_offset());
		}
		return position - border_offset;
	}
	return position;
}

Size2i Window::get_size_with_decorations() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_get_size_with_decorations(window_id);
	}
	if (visible && is_embedded() && !get_flag(Window::FLAG_BORDERLESS)) {
		Size2 border_size;
		if (theme_cache.embedded_border.is_valid()) {
			border_size = theme_cache.embedded_border->get_minimum_size();
		}
		if (theme_cache.embedded_unfocused_border.is_valid()) {
			border_size =
				border_size.max(theme_cache.embedded_unfocused_border->get_minimum_size());
		}
		return size + border_size;
	}
	return size;
}

Size2i Window::_clamp_limit_size(const Size2i& p_limit_size)
{
	// Force window limits to respect size limitations of rendering server.
	Size2i max_window_size = Renderer::get_maximum_viewport_size();
	if (max_window_size != Size2i()) {
		return p_limit_size.clamp(Vector2i(), max_window_size);
	}
	else {
		return p_limit_size.maxi(0);
	}
}

void Window::_validate_limit_size()
{
	// When max_size is invalid, max_size_used falls back to respect size limitations of rendering
	// server.
	bool max_size_valid =
		(max_size.x > 0 || max_size.y > 0) && max_size.x >= min_size.x && max_size.y >= min_size.y;
	max_size_used = max_size_valid ? max_size : Renderer::get_maximum_viewport_size();
}

void Window::set_max_size(const Size2i& p_max_size)
{
	ERR_MAIN_THREAD_GUARD;
#if defined(ANDROID_ENABLED)
	if (!get_parent() && is_inside_tree()) {
		// Can't set root window size on Android.
		return;
	}
#endif
	Size2i max_size_clamped = _clamp_limit_size(p_max_size);
	if (max_size == max_size_clamped) {
		return;
	}
	max_size = max_size_clamped;

	_validate_limit_size();
	_update_window_size();
}

Size2i Window::get_max_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	return max_size;
}

void Window::set_min_size(const Size2i& p_min_size)
{
	ERR_MAIN_THREAD_GUARD;
#if defined(ANDROID_ENABLED)
	if (!get_parent() && is_inside_tree()) {
		// Can't set root window size on Android.
		return;
	}
#endif
	Size2i min_size_clamped = _clamp_limit_size(p_min_size);
	if (min_size == min_size_clamped) {
		return;
	}
	min_size = min_size_clamped;

	_validate_limit_size();
	_update_window_size();
}

Size2i Window::get_min_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	return min_size;
}

void Window::set_mode(Mode p_mode)
{
	ERR_MAIN_THREAD_GUARD;
	mode = p_mode;

	if (embedder) {
		embedder->_sub_window_update(this);

	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_mode(
			DisplayServerEnums::WindowMode(p_mode), window_id);
	}
}

Window::Mode Window::get_mode() const
{
	ERR_READ_THREAD_GUARD_V(MODE_WINDOWED);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		mode = (Mode)DisplayServer::window_get_mode(window_id);
	}
	return mode;
}

void Window::set_fullscreen_shortcut_enabled(bool p_enabled)
{
	fullscreen_shortcut_enabled = p_enabled;
}

bool Window::is_fullscreen_shortcut_enabled() const { return fullscreen_shortcut_enabled; }

void Window::set_flag(Flags p_flag, bool p_enabled)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_flag, FLAG_MAX);
	flags[p_flag] = p_enabled;

	if (p_flag == FLAG_TRANSPARENT) {
		set_transparent_background(p_enabled);
	}

	if (embedder) {
		embedder->_sub_window_update(this);
	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		if (!is_in_edited_scene_root()) {
			DisplayServer::window_set_flag(
				DisplayServerEnums::WindowFlags(p_flag), p_enabled, window_id);
		}
	}
}

bool Window::get_flag(Flags p_flag) const
{
	ERR_READ_THREAD_GUARD_V(false);
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		if (!is_in_edited_scene_root()) {
			flags[p_flag] = DisplayServer::window_get_flag(
				DisplayServerEnums::WindowFlags(p_flag), window_id);
		}
	}
	return flags[p_flag];
}

bool Window::is_popup() const
{
	return (get_flag(Window::FLAG_POPUP) && get_flag(Window::FLAG_NO_FOCUS)) ||
		   get_flag(Window::FLAG_MOUSE_PASSTHROUGH);
}

void Window::set_hdr_output_requested(bool p_requested)
{
	ERR_MAIN_THREAD_GUARD;

	hdr_output_requested = p_requested;

	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_request_hdr_output(hdr_output_requested, window_id);
	}

	_update_viewport_for_hdr_output();
}

bool Window::is_hdr_output_requested() const
{
	ERR_READ_THREAD_GUARD_V(false);

	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		hdr_output_requested =
			DisplayServer::window_is_hdr_output_requested(window_id);
	}

	return hdr_output_requested;
}

float Window::get_output_max_linear_value() const
{
	ERR_READ_THREAD_GUARD_V(1.0f);

	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_get_output_max_linear_value(window_id);
	}

	return 1.0f;
}

bool Window::is_maximize_allowed() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_is_maximize_allowed(window_id);
	}
	return true;
}

void Window::request_attention()
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_request_attention(window_id);
	}
}

void Window::set_taskbar_progress_value(float p_value)
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_taskbar_progress_value(p_value, window_id);
	}
}

void Window::set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state)
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_taskbar_progress_state(p_state);
	}
}

#ifndef DISABLE_DEPRECATED
void Window::move_to_foreground()
{
	WARN_DEPRECATED_MSG(
		R"*(The "move_to_foreground()" method is deprecated, use "grab_focus()" instead.)*");
	grab_focus();
}
#endif // DISABLE_DEPRECATED

bool Window::can_draw() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (!is_inside_tree()) {
		return false;
	}
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_can_draw(window_id);
	}

	return visible;
}

void Window::set_ime_active(bool p_active)
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_ime_active(p_active, window_id);
	}
}

void Window::set_ime_position(const Point2i& p_pos)
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_ime_position(p_pos, window_id);
	}
}

bool Window::is_embedded() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return get_embedder() != nullptr;
}

bool Window::is_in_edited_scene_root() const
{
	ERR_READ_THREAD_GUARD_V(false);
#ifdef TOOLS_ENABLED
	return is_part_of_edited_scene();
#else
	return false;
#endif
}

void Window::_update_from_window()
{
	ERR_FAIL_COND(window_id == DisplayServerEnums::INVALID_WINDOW_ID);
	mode = (Mode)DisplayServer::window_get_mode(window_id);
	for (int i = 0; i < FLAG_MAX; i++) {
		flags[i] = DisplayServer::window_get_flag(
			DisplayServerEnums::WindowFlags(i), window_id);
	}
}

void Window::_clear_window()
{
	ERR_FAIL_COND(window_id == DisplayServerEnums::INVALID_WINDOW_ID);

	bool had_focus = has_focus();

	if (DisplayServer::has_feature(
			DisplayServerEnums::FEATURE_SELF_FITTING_WINDOWS)) {
		float win_scale = DisplayServer::window_get_scale(window_id);

		Size2i adjusted_size = Size2i(size.width / win_scale, size.height / win_scale);
		Size2i adjusted_pos = Size2i(position.x / win_scale, position.y / win_scale);

		_rect_changed_callback(Rect2i(adjusted_pos, adjusted_size));
	}

	if (transient_parent && transient_parent->window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_transient(
			window_id, DisplayServerEnums::INVALID_WINDOW_ID);
	}

	for (const Window* E : transient_children) {
		if (E->window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
			DisplayServer::window_set_transient(
				E->window_id, DisplayServerEnums::INVALID_WINDOW_ID);
		}
	}

	_update_from_window();

	DisplayServer::delete_sub_window(window_id);
	window_id = DisplayServerEnums::INVALID_WINDOW_ID;

	// If closing window was focused and has a parent, return focus.
	if (had_focus && transient_parent) {
		transient_parent->grab_focus();
	}

	_update_viewport_size();
	Renderer::viewport_set_update_mode(
		get_viewport_rid(), RSE::VIEWPORT_UPDATE_DISABLED);

	if (transient && transient_to_focused) {
		_clear_transient();
	}
}

void Window::_rect_changed_callback(const Rect2i& p_callback)
{
	// we must always accept this as the truth
	if (size == p_callback.size && position == p_callback.position) {
		return;
	}

	if (position != p_callback.position) {
		position = p_callback.position;
		_propagate_window_notification(this, NOTIFICATION_WM_POSITION_CHANGED);
	}

	if (size != p_callback.size) {
		size = p_callback.size;
		_update_viewport_size();
	}
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID &&
		!DisplayServer::has_feature(
			DisplayServerEnums::FEATURE_SELF_FITTING_WINDOWS)) {
		Vector2 sz_out =
			DisplayServer::window_get_size_with_decorations(window_id);
		Vector2 pos_out =
			DisplayServer::window_get_position_with_decorations(window_id);
		Vector2 sz_in = DisplayServer::window_get_size(window_id);
		Vector2 pos_in = DisplayServer::window_get_position(window_id);
		AccessibilityServer::set_window_rect(
			window_id, Rect2(pos_out, sz_out), Rect2(pos_in, sz_in));
	}
	queue_accessibility_update();
}

void Window::update_mouse_cursor_state()
{
	ERR_MAIN_THREAD_GUARD;
	// Update states based on mouse cursor position.
	// This includes updated mouse_enter or mouse_exit signals or the current mouse cursor shape.
	// These details are set in Viewport::_gui_input_event. To instantly
	// see the changes in the viewport, we need to trigger a mouse motion event.
	// This function should be called whenever scene tree changes affect the mouse cursor.
	InputEvent mm = InputEvent::make_mouse_motion();
	Vector2 pos = get_mouse_position();
	Transform2D xform = get_global_canvas_transform().affine_inverse();
	mm.mouse_motion.position = pos;
	mm.mouse_motion.global_position = xform.xform(pos);
	mm.device = InputEvent::DEVICE_ID_INTERNAL;
	push_input(mm, true);
}

void Window::show()
{
	ERR_MAIN_THREAD_GUARD;
	set_visible(true);
}

void Window::hide()
{
	ERR_MAIN_THREAD_GUARD;
	set_visible(false);
}

void Window::_accessibility_activate()
{
	_accessibility_notify_enter(this);
	if (!get_embedder()) {
		AccessibilityServer::window_activation_completed(get_window_id());
	}
}

void Window::_accessibility_deactivate()
{
	_accessibility_notify_exit(this);
	if (!get_embedder()) {
		AccessibilityServer::window_deactivation_completed(get_window_id());
	}
}

void Window::_clear_transient()
{
	if (transient_parent) {
		if (transient_parent->window_id != DisplayServerEnums::INVALID_WINDOW_ID &&
			window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
			DisplayServer::window_set_transient(
				window_id, DisplayServerEnums::INVALID_WINDOW_ID);
		}
		transient_parent->transient_children.erase(this);
		if (transient_parent->exclusive_child == this) {
			transient_parent->exclusive_child = nullptr;
		}
		transient_parent = nullptr;
	}
}

void Window::_set_transient_exclusive_child(bool p_clear_invalid)
{
	if (exclusive && visible && is_inside_tree()) {
		if (!is_in_edited_scene_root()) {
			// Transient parent has another exclusive child.
			if (transient_parent->exclusive_child && transient_parent->exclusive_child != this) {
				ERR_PRINT(vformat("Attempting to make child window exclusive, but the parent "
								  "window already has another exclusive child. This window, "
								  "parent window, and current exclusive child window"));
			}
			transient_parent->exclusive_child = this;
		}
	}
	else if (p_clear_invalid) {
		if (transient_parent->exclusive_child == this) {
			transient_parent->exclusive_child = nullptr;
		}
	}
}

void Window::set_transient(bool p_transient)
{
	ERR_MAIN_THREAD_GUARD;
	if (transient == p_transient) {
		return;
	}

	transient = p_transient;

	if (!is_inside_tree()) {
		return;
	}

	if (transient) {
		if (!transient_to_focused) {
			_make_transient();
		}
	}
	else {
		_clear_transient();
	}
}

bool Window::is_transient() const { return transient; }

void Window::set_transient_to_focused(bool p_transient_to_focused)
{
	ERR_MAIN_THREAD_GUARD;
	if (transient_to_focused == p_transient_to_focused) {
		return;
	}

	transient_to_focused = p_transient_to_focused;
}

bool Window::is_transient_to_focused() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return transient_to_focused;
}

void Window::set_exclusive(bool p_exclusive)
{
	ERR_MAIN_THREAD_GUARD;
	if (exclusive == p_exclusive) {
		return;
	}

	exclusive = p_exclusive;

	if (!embedder && window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		if (is_in_edited_scene_root()) {
			DisplayServer::window_set_exclusive(window_id, false);
		}
		else {
			DisplayServer::window_set_exclusive(window_id, exclusive);
		}
	}

	if (transient_parent) {
		_set_transient_exclusive_child(true);
	}
}

bool Window::is_exclusive() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return exclusive;
}

bool Window::is_visible() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return visible;
}

Size2i Window::_clamp_window_size(const Size2i& p_size)
{
	Size2i window_size_clamped = p_size;
	Size2 minsize = get_clamped_minimum_size();
	window_size_clamped = window_size_clamped.max(minsize);

	if (max_size_used != Size2i()) {
		window_size_clamped = window_size_clamped.min(max_size_used);
	}

	return window_size_clamped;
}

void Window::_update_window_size()
{
	Size2i size_limit = get_clamped_minimum_size();
	if (!embedder && window_id != DisplayServerEnums::INVALID_WINDOW_ID && keep_title_visible) {
		Size2i title_size =
			DisplayServer::window_get_title_size(displayed_title, window_id);
		size_limit = size_limit.max(title_size);
	}

	size = size.max(size_limit);

	bool reset_min_first = false;

	if (max_size_used != Size2i()) {
		// Force window size to respect size limitations of max_size_used.
		size = size.min(max_size_used);

		if (size_limit.x > max_size_used.x) {
			size_limit.x = max_size_used.x;
			reset_min_first = true;
		}
		if (size_limit.y > max_size_used.y) {
			size_limit.y = max_size_used.y;
			reset_min_first = true;
		}
	}

	if (embedder) {
		size = size.maxi(1);

		embedder->_sub_window_update(this);
	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		// When main window embedded in the editor, we can't resize the main window.
		if (window_id != DisplayServerEnums::MAIN_WINDOW_ID ||
			!Engine::is_embedded_in_editor()) {
			if (reset_min_first && wrap_controls) {
				// Avoid an error if setting max_size to a value between min_size and the previous
				// size_limit.
				DisplayServer::window_set_min_size(Size2i(), window_id);
			}

			DisplayServer::window_set_max_size(max_size_used, window_id);
			DisplayServer::window_set_min_size(size_limit, window_id);
			DisplayServer::window_set_size(size, window_id);
		}
		else if (Engine::is_embedded_in_editor()) {
			size = DisplayServer::window_get_size(window_id); // Reset size.
		}
	}

	// update the viewport
	_update_viewport_size();
}

void Window::set_force_native(bool p_force_native)
{
	if (force_native == p_force_native) {
		return;
	}
	if (is_visible() && !is_in_edited_scene_root()) {
		ERR_FAIL_MSG("Can't change \"force_native\" while a window is displayed. Consider hiding "
					 "window before changing this value.");
	}
	if (window_id == DisplayServerEnums::MAIN_WINDOW_ID) {
		return;
	}
	force_native = p_force_native;
	if (!is_in_edited_scene_root() && is_inside_tree() &&
		SceneTree::get_root()->is_embedding_subwindows()) {
		set_embedding_subwindows(force_native);
	}
}

bool Window::get_force_native() const { return force_native; }

Viewport* Window::get_embedder() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	if (force_native &&
		DisplayServer::has_feature(DisplayServerEnums::FEATURE_SUBWINDOWS) &&
		!is_in_edited_scene_root()) {
		return nullptr;
	}

	Viewport* vp = get_parent_viewport();

	while (vp) {
		if (vp->is_embedding_subwindows()) {
			return vp;
		}

		if (vp->get_parent()) {
			vp = vp->get_parent()->get_viewport();
		}
		else {
			vp = nullptr;
		}
	}
	return nullptr;
}

RID Window::get_accessibility_element() const
{
	if (!visible || is_part_of_edited_scene()) {
		return RID();
	}
	if (get_embedder() || is_popup()) {
		return Node::get_accessibility_element();
	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return AccessibilityServer::get_window_root(window_id);
	}
	else {
		return RID();
	}
}

RID Window::get_focused_accessibility_element() const
{
	if (window_id == DisplayServerEnums::MAIN_WINDOW_ID) {
		if (get_child_count() > 0) {
			return get_child(0)->get_focused_accessibility_element(); // Try scene tree root node.
		}
	}
	return Node::get_focused_accessibility_element();
}

String Window::_get_accessibility_name() const
{
	if (accessibility_name.is_empty()) {
		return displayed_title;
	}
	else {
		return accessibility_name;
	}
}

Transform2D Window::get_accessibility_transform() const
{
	if (is_inside_tree() && get_parent()) {
		Transform2D parent_tr = get_parent()->get_accessibility_transform();
		Transform2D window_tr;
		if (window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
			window_tr.set_origin(position);
		}
		else {
			Window* np = get_non_popup_window();
			if (np) {
				window_tr.set_origin(get_position() - np->get_position());
			}
		}
		window_tr.set_scale(Vector2(1.f, 1.f) * get_content_scale_factor());
		return parent_tr.affine_inverse() * window_tr;
	}
	else {
		return Transform2D();
	}
}

void Window::set_content_scale_size(const Size2i& p_size)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(p_size.x < 0);
	ERR_FAIL_COND(p_size.y < 0);
	content_scale_size = p_size;
	_update_viewport_size();
}

Size2i Window::get_content_scale_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	return content_scale_size;
}

void Window::set_content_scale_mode(ContentScaleMode p_mode)
{
	ERR_MAIN_THREAD_GUARD;
	content_scale_mode = p_mode;
	_update_viewport_size();
}

Window::ContentScaleMode Window::get_content_scale_mode() const
{
	ERR_READ_THREAD_GUARD_V(CONTENT_SCALE_MODE_DISABLED);
	return content_scale_mode;
}

void Window::set_content_scale_aspect(ContentScaleAspect p_aspect)
{
	ERR_MAIN_THREAD_GUARD;

	content_scale_aspect = p_aspect;
	_update_viewport_size();
}

Window::ContentScaleAspect Window::get_content_scale_aspect() const
{
	ERR_READ_THREAD_GUARD_V(CONTENT_SCALE_ASPECT_IGNORE);
	return content_scale_aspect;
}

void Window::set_content_scale_stretch(ContentScaleStretch p_stretch)
{
	content_scale_stretch = p_stretch;
	_update_viewport_size();
}

Window::ContentScaleStretch Window::get_content_scale_stretch() const
{
	return content_scale_stretch;
}

void Window::set_keep_title_visible(bool p_title_visible)
{
	if (keep_title_visible == p_title_visible) {
		return;
	}
	keep_title_visible = p_title_visible;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		_update_window_size();
	}
}

bool Window::get_keep_title_visible() const { return keep_title_visible; }

void Window::set_content_scale_factor(real_t p_factor)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(p_factor <= 0);
	content_scale_factor = p_factor;
	_update_viewport_size();
}

real_t Window::get_content_scale_factor() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return content_scale_factor;
}

void Window::set_nonclient_area(const Rect2i& p_rect)
{
	ERR_MAIN_THREAD_GUARD;
	nonclient_area = p_rect;
}

Rect2i Window::get_nonclient_area() const { return nonclient_area; }

DisplayServerEnums::WindowID Window::get_window_id() const
{
	ERR_READ_THREAD_GUARD_V(DisplayServerEnums::INVALID_WINDOW_ID);
	if (get_embedder()) {
#ifdef TOOLS_ENABLED
		if (is_part_of_edited_scene()) {
			return DisplayServerEnums::MAIN_WINDOW_ID;
		}
#endif
		return parent->get_window_id();
	}
	return window_id;
}

void Window::set_mouse_passthrough_polygon(const Vector<Vector2>& p_region)
{
	ERR_MAIN_THREAD_GUARD;
	mpath = p_region;
	if (window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}
	DisplayServer::window_set_mouse_passthrough(mpath, window_id);
}

Vector<Vector2> Window::get_mouse_passthrough_polygon() const { return mpath; }

void Window::set_wrap_controls(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	wrap_controls = p_enable;

	if (!is_inside_tree()) {
		return;
	}

	if (updating_child_controls) {
		_update_child_controls();
	}
	else {
		_update_window_size();
	}
}

void Window::child_controls_changed() {}

bool Window::is_wrapping_controls() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return wrap_controls;
}

void Window::_update_viewport_for_hdr_output()
{
	// If HDR output is enabled, we need to enable HDR 2D rendering as well.
	// This is required to get the correct dynamic range for the final output.
	// We only need to do this if the viewport is not already set up for HDR 2D rendering.

	if (!is_using_hdr_2d()) {
		Renderer::viewport_set_use_hdr_2d(viewport, hdr_output_requested);
	}
}

void Window::_update_child_controls()
{
	if (!updating_child_controls) {
		return;
	}

	_update_window_size();

	updating_child_controls = false;
}

bool Window::_can_consume_input_events() const { return exclusive_child == nullptr; }

void Window::_window_input_text(const String& p_text, bool p_emit_signal)
{
	_push_text_input(p_text, p_emit_signal);
}

Viewport* Window::get_parent_viewport() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	if (get_parent()) {
		return get_parent()->get_viewport();
	}
	else {
		return nullptr;
	}
}

Window* Window::get_non_popup_window() const
{
	Window* w = const_cast<Window*>(this);
	while (w && w->is_popup()) {
		w = w->get_parent_visible_window();
	}
	return w;
}

void Window::popup(const Rect2i& p_screen_rect) {}

void Window::popup_on_parent(const Rect2i& p_parent_rect)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!is_inside_tree());
	ERR_FAIL_COND_MSG(
		window_id == DisplayServerEnums::MAIN_WINDOW_ID, "Can't popup the main window.");

	if (!is_embedded()) {
		Window* window = get_parent_visible_window();

		if (!window) {
			popup(p_parent_rect);
		}
		else {
			popup(Rect2i(window->get_position() + p_parent_rect.position, p_parent_rect.size));
		}
	}
	else {
		popup(p_parent_rect);
	}
}

bool Window::_try_parent_dialog(Node* p_from_node)
{
	ERR_FAIL_NULL_V(p_from_node, false);
	ERR_FAIL_COND_V_MSG(is_inside_tree(), false,
		"Attempting to parent and popup a dialog that already has a parent.");

	Window* w = p_from_node->get_last_exclusive_window();
	if (w && w != this) {
		w->add_child(this);
		return true;
	}
	return false;
}

void Window::popup_centered(const Size2i& p_minsize) {}

void Window::popup_centered_ratio(float p_ratio) {}

void Window::popup_centered_clamped(const Size2i& p_size, float p_fallback_ratio) {}

void Window::popup_exclusive(Node* p_from_node, const Rect2i& p_screen_rect)
{
	if (_try_parent_dialog(p_from_node)) {
		popup(p_screen_rect);
	}
}

void Window::popup_exclusive_on_parent(Node* p_from_node, const Rect2i& p_parent_rect)
{
	if (_try_parent_dialog(p_from_node)) {
		popup_on_parent(p_parent_rect);
	}
}

void Window::popup_exclusive_centered_ratio(Node* p_from_node, float p_ratio)
{
	if (_try_parent_dialog(p_from_node)) {
		popup_centered_ratio(p_ratio);
	}
}

void Window::popup_exclusive_centered_clamped(
	Node* p_from_node, const Size2i& p_size, float p_fallback_ratio)
{
	if (_try_parent_dialog(p_from_node)) {
		popup_centered_clamped(p_size, p_fallback_ratio);
	}
}

Rect2i Window::fit_rect_in_parent(Rect2i p_rect, const Rect2i& p_parent_rect) const
{
	ERR_READ_THREAD_GUARD_V(Rect2i());
	Size2i limit = p_parent_rect.size;
	if (p_rect.position.x + p_rect.size.x > limit.x) {
		p_rect.position.x = limit.x - p_rect.size.x;
	}
	if (p_rect.position.y + p_rect.size.y > limit.y) {
		p_rect.position.y = limit.y - p_rect.size.y;
	}

	if (p_rect.position.x < 0) {
		p_rect.position.x = 0;
	}

	int title_height = get_flag(Window::FLAG_BORDERLESS) ? 0 : theme_cache.title_height;

	if (p_rect.position.y < title_height) {
		p_rect.position.y = title_height;
	}

	return p_rect;
}

Size2 Window::get_contents_minimum_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	return _get_contents_minimum_size();
}

Size2 Window::get_clamped_minimum_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	if (!wrap_controls) {
		return min_size;
	}

	return min_size.max(get_contents_minimum_size() * get_content_scale_factor());
}

void Window::grab_focus()
{
	ERR_MAIN_THREAD_GUARD;
	if (embedder) {
		embedder->_sub_window_grab_focus(this);
	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_move_to_foreground(window_id);
	}
}

bool Window::has_focus() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_is_focused(window_id);
	}
	return focused;
}

bool Window::has_focus_or_active_popup() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_is_focused(window_id) ||
			   (DisplayServer::window_get_active_popup() == window_id);
	}
	return focused;
}

void Window::start_drag()
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_start_drag(window_id);
	}
	else if (embedder) {
		embedder->_window_start_drag(this);
	}
}

void Window::start_resize(DisplayServerEnums::WindowResizeEdge p_edge)
{
	ERR_MAIN_THREAD_GUARD;
	if (get_flag(FLAG_RESIZE_DISABLED)) {
		return;
	}
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_start_resize(p_edge, window_id);
	}
	else if (embedder) {
		switch (p_edge) {
		case DisplayServerEnums::WINDOW_EDGE_TOP_LEFT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_TOP_LEFT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_TOP: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_TOP, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_TOP_RIGHT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_TOP_RIGHT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_LEFT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_LEFT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_RIGHT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_RIGHT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM_LEFT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_BOTTOM_LEFT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_BOTTOM, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM_RIGHT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_BOTTOM_RIGHT, this);
		} break;
		default:
			break;
		}
	}
}

Rect2i Window::get_usable_parent_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2i());
	ERR_FAIL_COND_V(!is_inside_tree(), Rect2());
	Rect2i parent_rect;
	if (is_embedded()) {
		parent_rect = get_embedder()->get_visible_rect();
	}
	else {
		const Window* w = is_visible() ? this : get_parent_visible_window();
		// find a parent that can contain us
		ERR_FAIL_NULL_V(w, Rect2());

		parent_rect = DisplayServer::screen_get_usable_rect(
			DisplayServer::window_get_current_screen(w->get_window_id()));
	}
	return parent_rect;
}

void Window::set_accessibility_description(const String& p_description)
{
	ERR_MAIN_THREAD_GUARD;
	if (accessibility_description != p_description) {
		accessibility_description = p_description;
		queue_accessibility_update();
	}
}

void Window::accessibility_announcement(const String& p_announcement)
{
	ERR_MAIN_THREAD_GUARD;
	announcement = p_announcement;
	queue_accessibility_update();
}

void Window::add_child_notify(Node* p_child)
{
	if (is_inside_tree() && wrap_controls) {
		child_controls_changed();
	}
}

void Window::remove_child_notify(Node* p_child)
{
	if (is_inside_tree() && wrap_controls) {
		child_controls_changed();
	}
}

// Theming.

void Window::set_theme_owner_node(Node* p_node)
{
	ERR_MAIN_THREAD_GUARD;
	theme_owner->set_owner_node(p_node);
}

Node* Window::get_theme_owner_node() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return theme_owner->get_owner_node();
}

bool Window::has_theme_owner_node() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return theme_owner->has_owner_node();
}

void Window::set_theme_context(ThemeContext* p_context, bool p_propagate)
{
	ERR_MAIN_THREAD_GUARD;
	theme_owner->set_owner_context(p_context, p_propagate);
}

Ref<Theme> Window::get_theme() const
{
	ERR_READ_THREAD_GUARD_V(Ref<Theme>());
	return theme;
}

void Window::_theme_changed()
{
	if (is_inside_tree()) {
		theme_owner->propagate_theme_changed(this, this, true, false);
	}
}

void Window::_invalidate_theme_cache()
{
	theme_icon_cache.clear();
	theme_style_cache.clear();
	theme_font_cache.clear();
	theme_font_size_cache.clear();
	theme_color_cache.clear();
	theme_constant_cache.clear();
}

void Window::_update_embedded_window()
{
	if (!updating_embedded_window) {
		return;
	}

	if (embedder) {
		embedder->_sub_window_update(this);
	};

	updating_embedded_window = false;
}

String Window::get_theme_type_variation() const
{
	ERR_READ_THREAD_GUARD_V(String());
	return theme_type_variation;
}

/// Theme property lookup.

#ifdef TOOLS_ENABLED
Ref<Texture2D> Window::get_editor_theme_icon(const String& p_name) const
{
	return get_theme_icon(p_name, SNAME("EditorIcons"));
}
#endif

void Window::add_theme_font_size_override(const String& p_name, int p_font_size)
{
	ERR_MAIN_THREAD_GUARD;
	theme_font_size_override[p_name] = p_font_size;
	_notify_theme_override_changed();
}

void Window::add_theme_color_override(const String& p_name, const Color& p_color)
{
	ERR_MAIN_THREAD_GUARD;
	theme_color_override[p_name] = p_color;
	_notify_theme_override_changed();
}

void Window::add_theme_constant_override(const String& p_name, int p_constant)
{
	ERR_MAIN_THREAD_GUARD;
	theme_constant_override[p_name] = p_constant;
	_notify_theme_override_changed();
}

void Window::remove_theme_font_size_override(const String& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	theme_font_size_override.erase(p_name);
	_notify_theme_override_changed();
}

void Window::remove_theme_color_override(const String& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	theme_color_override.erase(p_name);
	_notify_theme_override_changed();
}

Ref<StyleBox> Window::get_theme_stylebox(const String& p_name, const String& p_theme_type) const
{
	Ref<StyleBox> sb = memnew(StyleBox);
	return sb;
}

void Window::remove_theme_constant_override(const String& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	theme_constant_override.erase(p_name);
	_notify_theme_override_changed();
}

bool Window::has_theme_icon_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<Texture2D>* tex = theme_icon_override.getptr(p_name);
	return tex != nullptr;
}

bool Window::has_theme_stylebox_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<StyleBox>* style = theme_style_override.getptr(p_name);
	return style != nullptr;
}

bool Window::has_theme_font_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<Font>* font = theme_font_override.getptr(p_name);
	return font != nullptr;
}

bool Window::has_theme_font_size_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const int* font_size = theme_font_size_override.getptr(p_name);
	return font_size != nullptr;
}

bool Window::has_theme_color_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Color* color = theme_color_override.getptr(p_name);
	return color != nullptr;
}

bool Window::has_theme_constant_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const int* constant = theme_constant_override.getptr(p_name);
	return constant != nullptr;
}

/// Default theme properties.

float Window::get_theme_default_base_scale() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return theme_owner->get_theme_default_base_scale();
}

Ref<Font> Window::get_theme_default_font() const
{
	ERR_READ_THREAD_GUARD_V(Ref<Font>());
	return theme_owner->get_theme_default_font();
}

int Window::get_theme_default_font_size() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return theme_owner->get_theme_default_font_size();
}

/// Bulk actions.

void Window::begin_bulk_theme_override()
{
	ERR_MAIN_THREAD_GUARD;
	bulk_theme_override = true;
}

void Window::end_bulk_theme_override()
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!bulk_theme_override);

	bulk_theme_override = false;
	_notify_theme_override_changed();
}

//

Rect2i Window::get_parent_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2i());
	ERR_FAIL_COND_V(!is_inside_tree(), Rect2i());
	if (is_embedded()) {
		// viewport
		Node* n = get_parent();
		ERR_FAIL_NULL_V(n, Rect2i());
		Viewport* p = n->get_viewport();
		ERR_FAIL_NULL_V(p, Rect2i());

		return p->get_visible_rect();
	}
	else {
		int x = get_position().x;
		int closest_dist = 0x7FFFFFFF;
		Rect2i closest_rect;
		for (int i = 0; i < DisplayServer::get_screen_count(); i++) {
			Rect2i s(DisplayServer::screen_get_position(i),
				DisplayServer::screen_get_size(i));
			int d;
			if (x >= s.position.x && x < s.size.x) {
				// contained
				closest_rect = s;
				break;
			}
			else if (x < s.position.x) {
				d = s.position.x - x;
			}
			else {
				d = x - (s.position.x + s.size.x);
			}

			if (d < closest_dist) {
				closest_dist = d;
				closest_rect = s;
			}
		}
		return closest_rect;
	}
}

void Window::set_clamp_to_embedder(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	clamp_to_embedder = p_enable;
}

bool Window::is_clamped_to_embedder() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return clamp_to_embedder;
}

void Window::set_unparent_when_invisible(bool p_unparent) { unparent_when_invisible = p_unparent; }

void Window::set_layout_direction(Window::LayoutDirection p_direction)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_direction, LAYOUT_DIRECTION_MAX);

	layout_dir = p_direction;
	propagate_notification(Control::NOTIFICATION_LAYOUT_DIRECTION_CHANGED);
}

Window::LayoutDirection Window::get_layout_direction() const
{
	ERR_READ_THREAD_GUARD_V(LAYOUT_DIRECTION_INHERITED);
	return layout_dir;
}

#ifndef DISABLE_DEPRECATED

void Window::set_use_font_oversampling(bool p_oversampling)
{
	Viewport::set_use_oversampling(p_oversampling);
}

bool Window::is_using_font_oversampling() const { return Viewport::is_using_oversampling(); }

void Window::set_auto_translate(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	set_auto_translate_mode(p_enable ? AUTO_TRANSLATE_MODE_ALWAYS : AUTO_TRANSLATE_MODE_DISABLED);
}

bool Window::is_auto_translating() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return can_auto_translate();
}
#endif

Transform2D Window::get_final_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return window_transform * stretch_transform * global_canvas_transform;
}

Transform2D Window::get_screen_transform_internal(bool p_absolute_position) const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	Transform2D embedder_transform;
	if (get_embedder()) {
		embedder_transform.translate_local(get_position());
		embedder_transform =
			get_embedder()->get_screen_transform_internal(p_absolute_position) * embedder_transform;
	}
	else if (p_absolute_position) {
		embedder_transform.translate_local(get_position());
	}
	return embedder_transform * get_final_transform();
}

Transform2D Window::get_popup_base_transform_native() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	if (!DisplayServer::has_feature(DisplayServerEnums::FEATURE_SUBWINDOWS)) {
		return Transform2D();
	}
	Transform2D popup_base_transform;
	popup_base_transform.set_origin(get_position());
	popup_base_transform *= get_final_transform();
	if (get_embedder()) {
		return get_embedder()->get_popup_base_transform_native() * popup_base_transform;
	}
	return popup_base_transform;
}

Transform2D Window::get_popup_base_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	if (is_embedding_subwindows()) {
		return Transform2D();
	}
	Transform2D popup_base_transform;
	popup_base_transform.set_origin(get_position());
	popup_base_transform *= get_final_transform();
	if (get_embedder()) {
		return get_embedder()->get_popup_base_transform() * popup_base_transform;
	}
	return popup_base_transform;
}

Viewport* Window::get_section_root_viewport() const
{
	if (get_embedder()) {
		return get_embedder()->get_section_root_viewport();
	}
	if (is_inside_tree()) {
		// Native window.
		return SceneTree::get_root();
	}
	Window* vp = const_cast<Window*>(this);
	return vp;
}

bool Window::is_attached_in_viewport() const { return get_embedder(); }

void Window::_update_mouse_over(Vector2 p_pos) {}

void Window::_mouse_leave_viewport()
{
	Viewport::_mouse_leave_viewport();
	if (is_embedded()) {
		mouse_in_window = false;
		_propagate_window_notification(this, NOTIFICATION_WM_MOUSE_EXIT);
	}
}

Window::Window()
{
	if (Renderer::data) {
		max_size = Renderer::get_maximum_viewport_size();
		max_size_used = max_size; // Update max_size_used.
	}

	theme_owner = memnew(ThemeOwner(this));
	Renderer::viewport_set_update_mode(
		get_viewport_rid(), RSE::VIEWPORT_UPDATE_DISABLED);
}

void Window::_update_viewport_size() {}

Color Window::get_theme_color(const String& p_name, const String& p_theme_type) const
{
	return Color();
}

Window* Window::get_parent_visible_window() const {}

void Window::_notify_theme_override_changed() {}

bool Window::is_layout_rtl() const {}


void Window::set_visible(bool) {}

Ref<Font> Window::get_theme_font(String const&, String const&) const {}

Window* Window::get_from_id(int) {}

void Window::_window_input(const InputEvent&) {}

void Window::_propagate_window_notification(Node*, int) {}

void Window::_popup_base(Rect2i const&) {}

int Window::get_theme_font_size(String const&, String const&) const {}

void Window::_update_theme_item_cache() {}

void Window::_settings_changed() {}

void Window::_make_transient() {}

Ref<Texture2D> Window::get_theme_icon(String const&, String const&) const {}

void Window::_event_callback(DisplayServerEnums::WindowEvent) {}

void Window::add_theme_font_override(String const&, Ref<Font> const&) {}

void Window::_accessibility_notify_exit(Node*) {}

void Window::_accessibility_notify_enter(Node*) {}

Size2 Window::_get_contents_minimum_size() const { return Size2(); }


