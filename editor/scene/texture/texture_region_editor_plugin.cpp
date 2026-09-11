/**************************************************************************/
/*  texture_region_editor_plugin.cpp                                      */
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

#include "core/input/input.h"
#include "core/os/keyboard.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/sprite_2d.h"
#include "scene/3d/sprite_3d.h"
#include "scene/gui/nine_patch_rect.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/view_panner.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/style_box_texture.h"
#include "servers/rendering/rendering_server.h"
#include "texture_region_editor_plugin.h"

Transform2D TextureRegionEditor::_get_offset_transform() const
{
	Transform2D mtx;
	mtx.columns[2] = -draw_ofs * draw_zoom;
	mtx.scale_basis(Vector2(draw_zoom, draw_zoom));

	return mtx;
}

void TextureRegionEditor::_texture_preview_draw()
{
	const Ref<Texture2D> object_texture = _get_edited_object_texture();
	if (object_texture.is_null()) {
		return;
	}

	Transform2D mtx = _get_offset_transform();

	RS::get_singleton()->canvas_item_add_set_transform(texture_preview->get_canvas_item(), mtx);
	texture_preview->draw_rect(
		Rect2(Point2(), object_texture->get_size()), Color(0.5, 0.5, 0.5, 0.5), false);
	texture_preview->draw_texture(object_texture.ptr(), Point2());
	RS::get_singleton()->canvas_item_add_set_transform(
		texture_preview->get_canvas_item(), Transform2D());
}

void TextureRegionEditor::_draw_margin_line(Vector2 p_from, Vector2 p_to)
{
	// Margin line is a dashed line with a normalized dash length. This method works
	// for both vertical and horizontal lines.

	Vector2 dash_size = (p_to - p_from).normalized() * 10;
	const int dash_thickness = Math::round(2 * EDSCALE);
	const Color dash_color = get_theme_color(SNAME("mono_color"), EditorStringName(Editor));
	const Color dash_bg_color = dash_color.inverted() * Color(1, 1, 1, 0.5);
	const int line_threshold = 200;

	// Draw a translucent background line to make the foreground line visible on any background.
	texture_overlay->draw_line(p_from, p_to, dash_bg_color, dash_thickness);

	Vector2 dash_start = p_from;
	while (dash_start.distance_squared_to(p_to) > line_threshold) {
		texture_overlay->draw_line(dash_start, dash_start + dash_size, dash_color, dash_thickness);

		// Skip two size lengths, one for the drawn dash and one for the gap.
		dash_start += dash_size * 2;
	}
}

void TextureRegionEditor::_set_grid_parameters_clamping(bool p_enabled)
{
	sb_off_x->set_allow_lesser(!p_enabled);
	sb_off_x->set_allow_greater(!p_enabled);
	sb_off_y->set_allow_lesser(!p_enabled);
	sb_off_y->set_allow_greater(!p_enabled);
	sb_step_x->set_allow_greater(!p_enabled);
	sb_step_y->set_allow_greater(!p_enabled);
	sb_sep_x->set_allow_greater(!p_enabled);
	sb_sep_y->set_allow_greater(!p_enabled);
}

int TextureRegionEditor::_get_overlapping_selection_handle(const Point2& p_mouse_pos)
{
	const real_t handle_radius = (16 * EDSCALE) / draw_zoom;
	const real_t handle_offset = (8 * EDSCALE) / draw_zoom;

	// Position of selection handles.
	const Vector2 endpoints[8] = {rect.position + Vector2(-handle_offset, -handle_offset),
		rect.position + Vector2(rect.size.x / 2, 0) + Vector2(0, -handle_offset),
		rect.position + Vector2(rect.size.x, 0) + Vector2(handle_offset, -handle_offset),
		rect.position + Vector2(rect.size.x, rect.size.y / 2) + Vector2(handle_offset, 0),
		rect.position + rect.size + Vector2(handle_offset, handle_offset),
		rect.position + Vector2(rect.size.x / 2, rect.size.y) + Vector2(0, handle_offset),
		rect.position + Vector2(0, rect.size.y) + Vector2(-handle_offset, handle_offset),
		rect.position + Vector2(0, rect.size.y / 2) + Vector2(-handle_offset, 0)};

	const Point2 mouse_pos = _get_offset_transform().affine_inverse().xform(p_mouse_pos);
	for (int i = 0; i < 8; i++) {
		if (mouse_pos.distance_to(endpoints[i]) <= handle_radius) {
			return i;
		}
	}

	return -1;
}

int TextureRegionEditor::_get_overlapping_margin_line(const Point2& p_mouse_pos, float* r_margin)
{
	float margins[4] = {};
	if (node_ninepatch) {
		margins[0] = node_ninepatch->get_patch_margin(SIDE_TOP);
		margins[1] = node_ninepatch->get_patch_margin(SIDE_BOTTOM);
		margins[2] = node_ninepatch->get_patch_margin(SIDE_LEFT);
		margins[3] = node_ninepatch->get_patch_margin(SIDE_RIGHT);
	}
	else if (res_stylebox.is_valid()) {
		margins[0] = res_stylebox->get_texture_margin(SIDE_TOP);
		margins[1] = res_stylebox->get_texture_margin(SIDE_BOTTOM);
		margins[2] = res_stylebox->get_texture_margin(SIDE_LEFT);
		margins[3] = res_stylebox->get_texture_margin(SIDE_RIGHT);
	}

	Vector2 pos[4] = {rect.position + Vector2(0, margins[0]),
		rect.position + rect.size - Vector2(0, margins[1]), rect.position + Vector2(margins[2], 0),
		rect.position + rect.size - Vector2(margins[3], 0)};

	const Point2 mouse_pos = _get_offset_transform().affine_inverse().xform(p_mouse_pos);
	const real_t line_thickness = (8 * EDSCALE) / draw_zoom;

	int margin_index = -1;
	if (Math::abs(mouse_pos.y - pos[0].y) <= line_thickness) {
		margin_index = 0;
	}
	else if (Math::abs(mouse_pos.y - pos[1].y) <= line_thickness) {
		margin_index = 1;
	}
	else if (Math::abs(mouse_pos.x - pos[2].x) <= line_thickness) {
		margin_index = 2;
	}
	else if (Math::abs(mouse_pos.x - pos[3].x) <= line_thickness) {
		margin_index = 3;
	}

	if (r_margin && margin_index >= 0) {
		*r_margin = margins[margin_index];
	}

	return margin_index;
}

void TextureRegionEditor::_pan_callback(Vector2 p_scroll_vec, Ref<InputEvent> p_event)
{
	p_scroll_vec /= draw_zoom;
	hscroll->set_value(hscroll->get_value() - p_scroll_vec.x);
	vscroll->set_value(vscroll->get_value() - p_scroll_vec.y);
}

void TextureRegionEditor::_zoom_callback(
	float p_zoom_factor, Vector2 p_origin, Ref<InputEvent> p_event)
{
	_zoom_on_position(draw_zoom * p_zoom_factor, p_origin);
}

void TextureRegionEditor::_input_from_window(const Ref<InputEvent>& p_event)
{
	if (!drag && p_event.is_valid() &&
		p_event->is_action_pressed(SNAME("ui_cancel"), false, true)) {
		hide();
	}
}

void TextureRegionEditor::_scroll_changed(float)
{
	if (updating_scroll) {
		return;
	}

	draw_ofs.x = hscroll->get_value();
	draw_ofs.y = vscroll->get_value();

	texture_preview->queue_redraw();
	texture_overlay->queue_redraw();
}

void TextureRegionEditor::_set_snap_mode(int p_mode)
{
	snap_mode = (SnapMode)p_mode;

	hb_grid->set_visible(snap_mode == SNAP_GRID);
	if (snap_mode == SNAP_AUTOSLICE && is_visible() && autoslice_is_dirty) {
		_update_autoslice();
	}

	texture_overlay->queue_redraw();
}

void TextureRegionEditor::_set_snap_off_x(float p_val)
{
	snap_offset.x = p_val;
	texture_overlay->queue_redraw();
}

void TextureRegionEditor::_set_snap_off_y(float p_val)
{
	snap_offset.y = p_val;
	texture_overlay->queue_redraw();
}

void TextureRegionEditor::_set_snap_step_x(float p_val)
{
	snap_step.x = p_val;
	texture_overlay->queue_redraw();
}

void TextureRegionEditor::_set_snap_step_y(float p_val)
{
	snap_step.y = p_val;
	texture_overlay->queue_redraw();
}

void TextureRegionEditor::_set_snap_sep_x(float p_val)
{
	snap_separation.x = p_val;
	texture_overlay->queue_redraw();
}

void TextureRegionEditor::_set_snap_sep_y(float p_val)
{
	snap_separation.y = p_val;
	texture_overlay->queue_redraw();
}

void TextureRegionEditor::_zoom_on_position(float p_zoom, Point2 p_position)
{
	if (p_zoom < min_draw_zoom || p_zoom > max_draw_zoom) {
		return;
	}

	float prev_zoom = draw_zoom;
	draw_zoom = p_zoom;
	Point2 ofs = p_position;
	ofs = ofs / prev_zoom - ofs / draw_zoom;
	draw_ofs = (draw_ofs + ofs).round();

	texture_preview->queue_redraw();
	texture_overlay->queue_redraw();
}

void TextureRegionEditor::_zoom_in()
{
	_zoom_on_position(draw_zoom * 1.5, texture_overlay->get_size() / 2.0);
}

void TextureRegionEditor::_zoom_reset()
{
	_zoom_on_position(1.0, texture_overlay->get_size() / 2.0);
}

void TextureRegionEditor::_zoom_out()
{
	_zoom_on_position(draw_zoom / 1.5, texture_overlay->get_size() / 2.0);
}

void TextureRegionEditor::_apply_rect(const Rect2& p_rect)
{
	if (node_sprite_2d) {
		node_sprite_2d->set_region_rect(p_rect);
	}
	else if (node_sprite_3d) {
		node_sprite_3d->set_region_rect(p_rect);
	}
	else if (node_ninepatch) {
		node_ninepatch->set_region_rect(p_rect);
	}
	else if (res_stylebox.is_valid()) {
		res_stylebox->set_region_rect(p_rect);
	}
	else if (res_atlas_texture.is_valid()) {
		res_atlas_texture->set_region(p_rect);
	}
}

void TextureRegionEditor::_update_rect() { rect = _get_edited_object_region(); }

void TextureRegionEditor::_update_autoslice()
{
	autoslice_is_dirty = false;
	autoslice_cache.clear();

	const Ref<Texture2D> object_texture = _get_edited_object_texture();
	if (object_texture.is_null()) {
		return;
	}

	for (int y = 0; y < object_texture->get_height(); y++) {
		for (int x = 0; x < object_texture->get_width(); x++) {
			if (object_texture->is_pixel_opaque(x, y)) {
				bool found = false;
				for (Rect2& E : autoslice_cache) {
					Rect2 grown = E.grow(1.5);
					if (grown.has_point(Point2(x, y))) {
						E.expand_to(Point2(x, y));
						E.expand_to(Point2(x + 1, y + 1));
						x = E.position.x + E.size.x - 1;
						bool merged = true;
						while (merged) {
							merged = false;
							bool queue_erase = false;
							for (List<Rect2>::Element* F = autoslice_cache.front(); F;
								 F = F->next()) {
								if (queue_erase) {
									autoslice_cache.erase(F->prev());
									queue_erase = false;
								}
								if (F->get() == E) {
									continue;
								}
								if (E.grow(1).intersects(F->get())) {
									E.expand_to(F->get().position);
									E.expand_to(F->get().position + F->get().size);
									if (F->prev()) {
										F = F->prev();
										autoslice_cache.erase(F->next());
									}
									else {
										queue_erase = true;
										// Can't delete the first rect in the list.
									}
									merged = true;
								}
							}
						}
						found = true;
						break;
					}
				}
				if (!found) {
					Rect2 new_rect(x, y, 1, 1);
					autoslice_cache.push_back(new_rect);
				}
			}
		}
	}
	cache_map[object_texture->get_rid()] = autoslice_cache;
}

void TextureRegionEditor::_node_removed(Node* p_node)
{
	if (p_node == node_sprite_2d || p_node == node_sprite_3d || p_node == node_ninepatch) {
		_clear_edited_object();
		hide();
	}
}

Ref<Texture2D> TextureRegionEditor::_get_edited_object_texture() const
{
	if (node_sprite_2d) {
		return node_sprite_2d->get_texture();
	}
	if (node_sprite_3d) {
		return node_sprite_3d->get_texture();
	}
	if (node_ninepatch) {
		return node_ninepatch->get_texture();
	}
	if (res_stylebox.is_valid()) {
		return res_stylebox->get_texture();
	}
	if (res_atlas_texture.is_valid()) {
		return res_atlas_texture->get_atlas();
	}

	return Ref<Texture2D>();
}

Rect2 TextureRegionEditor::_get_edited_object_region() const
{
	Rect2 region;

	if (node_ninepatch) {
		region = node_ninepatch->get_region_rect();
	}
	else if (res_stylebox.is_valid()) {
		region = res_stylebox->get_region_rect();
	}
	else if (res_atlas_texture.is_valid()) {
		region = res_atlas_texture->get_region();
	}
	else if (node_sprite_2d) {
		region = node_sprite_2d->get_region_rect();
	}
	else if (node_sprite_3d) {
		region = node_sprite_3d->get_region_rect();
	}

	const Ref<Texture2D> object_texture = _get_edited_object_texture();
	if (region == Rect2() && object_texture.is_valid()) {
		region = Rect2(Vector2(), object_texture->get_size());
	}

	return region;
}

void TextureRegionEditor::_texture_changed()
{
	if (!is_visible()) {
		return;
	}
	_edit_region();
}

void TextureRegionEditor::_edit_region()
{
	const Ref<Texture2D> object_texture = _get_edited_object_texture();
	if (object_texture.is_null()) {
		_set_grid_parameters_clamping(false);
		_zoom_reset();
		hscroll->hide();
		vscroll->hide();
		texture_preview->queue_redraw();
		texture_overlay->queue_redraw();
		return;
	}

	CanvasItem::TextureFilter filter = CanvasItem::TEXTURE_FILTER_NEAREST_WITH_MIPMAPS;
	if (node_ninepatch) {
		filter = node_ninepatch->get_texture_filter_in_tree();
	}
	else if (node_sprite_2d) {
		filter = node_sprite_2d->get_texture_filter_in_tree();
	}
	else if (node_sprite_3d) {
		StandardMaterial3D::TextureFilter filter_3d = node_sprite_3d->get_texture_filter();

		switch (filter_3d) {
		case StandardMaterial3D::TEXTURE_FILTER_NEAREST:
			filter = CanvasItem::TEXTURE_FILTER_NEAREST;
			break;
		case StandardMaterial3D::TEXTURE_FILTER_LINEAR:
			filter = CanvasItem::TEXTURE_FILTER_LINEAR;
			break;
		case StandardMaterial3D::TEXTURE_FILTER_NEAREST_WITH_MIPMAPS:
			filter = CanvasItem::TEXTURE_FILTER_NEAREST_WITH_MIPMAPS;
			break;
		case StandardMaterial3D::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS:
			filter = CanvasItem::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS;
			break;
		case StandardMaterial3D::TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC:
			filter = CanvasItem::TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC;
			break;
		case StandardMaterial3D::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC:
			filter = CanvasItem::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC;
			break;
		default:
			// fallback to project default
			filter = CanvasItem::TEXTURE_FILTER_PARENT_NODE;
			break;
		}
	}

	// occurs when get_texture_filter_in_tree reaches the scene root
	if (filter == CanvasItem::TEXTURE_FILTER_PARENT_NODE) {
		SubViewport* root = EditorNode::get_singleton()->get_scene_root();

		if (root != nullptr) {
			Viewport::DefaultCanvasItemTextureFilter filter_default =
				root->get_default_canvas_item_texture_filter();

			// depending on default filter, set filter to match, otherwise fall back on nearest w/
			// mipmaps
			switch (filter_default) {
			case DEFAULT_CANVAS_ITEM_TEXTURE_FILTER_NEAREST:
				filter = CanvasItem::TEXTURE_FILTER_NEAREST;
				break;
			case DEFAULT_CANVAS_ITEM_TEXTURE_FILTER_LINEAR:
				filter = CanvasItem::TEXTURE_FILTER_LINEAR;
				break;
			case DEFAULT_CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS:
				filter = CanvasItem::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS;
				break;
			case DEFAULT_CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS:
			default:
				filter = CanvasItem::TEXTURE_FILTER_NEAREST_WITH_MIPMAPS;
				break;
			}
		}
		else {
			filter = CanvasItem::TEXTURE_FILTER_NEAREST_WITH_MIPMAPS;
		}
	}

	texture_preview->set_texture_filter(filter);
	texture_preview->set_texture_repeat(CanvasItem::TEXTURE_REPEAT_DISABLED);

	if (cache_map.has(object_texture->get_rid())) {
		autoslice_cache = cache_map[object_texture->get_rid()];
		autoslice_is_dirty = false;
	}
	else {
		if (is_visible() && snap_mode == SNAP_AUTOSLICE) {
			_update_autoslice();
		}
		else {
			autoslice_is_dirty = true;
		}
	}

	// Avoiding clamping with mismatched min/max.
	_set_grid_parameters_clamping(false);
	const Size2 tex_size = object_texture->get_size();
	sb_off_x->set_min(-tex_size.x);
	sb_off_x->set_max(tex_size.x);
	sb_off_y->set_min(-tex_size.y);
	sb_off_y->set_max(tex_size.y);
	sb_step_x->set_max(tex_size.x);
	sb_step_y->set_max(tex_size.y);
	sb_sep_x->set_max(tex_size.x);
	sb_sep_y->set_max(tex_size.y);

	_set_grid_parameters_clamping(true);
	sb_off_x->set_value(snap_offset.x);
	sb_off_y->set_value(snap_offset.y);
	sb_step_x->set_value(snap_step.x);
	sb_step_y->set_value(snap_step.y);
	sb_sep_x->set_value(snap_separation.x);
	sb_sep_y->set_value(snap_separation.y);

	_update_rect();
	texture_preview->queue_redraw();
	texture_overlay->queue_redraw();
}

Vector2 TextureRegionEditor::snap_point(Vector2 p_target) const
{
	if (snap_mode == SNAP_GRID) {
		p_target.x =
			Math::snap_scalar_separation(snap_offset.x, snap_step.x, p_target.x, snap_separation.x);
		p_target.y =
			Math::snap_scalar_separation(snap_offset.y, snap_step.y, p_target.y, snap_separation.y);
	}

	return p_target;
}

EditorInspectorPluginTextureRegion::EditorInspectorPluginTextureRegion()
{
	texture_region_editor = memnew(TextureRegionEditor);
	EditorNode::get_singleton()->get_gui_base()->add_child(texture_region_editor);
}

TextureRegionEditorPlugin::TextureRegionEditorPlugin()
{
	Ref<EditorInspectorPluginTextureRegion> inspector_plugin;
	inspector_plugin.instantiate();
	add_inspector_plugin(inspector_plugin);
}

Control::CursorShape TextureRegionEditorOverlay::get_cursor_shape(const Point2& p_pos) const
{
	int drag_index = editor->drag_index != -1 ? editor->drag_index
											  : editor->_get_overlapping_selection_handle(p_pos);
	switch (drag_index) {
	case 0:
	case 4:
		return CURSOR_FDIAGSIZE;
	case 2:
	case 6:
		return CURSOR_BDIAGSIZE;
	case 1:
	case 5:
		return CURSOR_VSIZE;
	case 3:
	case 7:
		return CURSOR_HSIZE;
	default:
		break;
	}

	int margin_index = editor->edited_margin != -1 ? editor->edited_margin
												   : editor->_get_overlapping_margin_line(p_pos);
	if (editor->node_ninepatch) {
		switch (margin_index) {
		case 0:
			return CURSOR_VSIZE;
		case 1:
			return CURSOR_VSIZE;
		case 2:
			return CURSOR_HSIZE;
		case 3:
			return CURSOR_HSIZE;
		default:
			break;
		};
	}

	Transform2D mtx = editor->_get_offset_transform();
	Vector2 point = mtx.affine_inverse().xform(p_pos);

	if (editor->rect.has_point(point) && editor->snap_mode != TextureRegionEditor::SNAP_AUTOSLICE) {
		return CURSOR_DRAG;
	}

	return CURSOR_ARROW;
}


