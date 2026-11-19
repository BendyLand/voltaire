/**************************************************************************/
/*  light_2d.cpp                                                          */
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

#include "light_2d.h"

#include "core/config/engine.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"

void Light2D::owner_changed_notify()
{
	_update_light_visibility();
}

void Light2D::_update_light_visibility()
{
	if (!is_inside_tree()) {
		return;
	}

	bool editor_ok = true;

#ifdef TOOLS_ENABLED
	if (editor_only) {
		if (!Engine::is_editor_hint()) {
			editor_ok = false;
		}
		else {
			editor_ok = (SceneTree::get_edited_scene_root() &&
						 (this == SceneTree::get_edited_scene_root() ||
							 get_owner() == SceneTree::get_edited_scene_root()));
		}
	}
#else
	if (editor_only) {
		editor_ok = false;
	}
#endif // TOOLS_ENABLED

	Renderer::canvas_light_set_enabled(
		canvas_light, enabled && is_visible_in_tree() && editor_ok);
}

void Light2D::set_light_type(LightType p_type)
{
	light_type = p_type;
	if (!canvas_light.is_valid() || !Renderer::data) {
		return;
	}
	if (light_type == LIGHT_POINT) {
		Renderer::canvas_light_set_mode(canvas_light, RSE::CANVAS_LIGHT_MODE_POINT);
	}
	else {
		Renderer::canvas_light_set_mode(canvas_light, RSE::CANVAS_LIGHT_MODE_DIRECTIONAL);
		Renderer::canvas_light_set_directional_distance(canvas_light, max_distance);
	}
}

void Light2D::set_enabled(bool p_enabled)
{
	enabled = p_enabled;
	_update_light_visibility();
}

bool Light2D::is_enabled() const { return enabled; }

void Light2D::set_editor_only(bool p_editor_only)
{
	editor_only = p_editor_only;
	_update_light_visibility();
}

bool Light2D::is_editor_only() const { return editor_only; }

void Light2D::set_color(const Color& p_color)
{
	color = p_color;
	Renderer::canvas_light_set_color(canvas_light, color);
}

Color Light2D::get_color() const { return color; }

void Light2D::set_height(real_t p_height)
{
	height = p_height;
	Renderer::canvas_light_set_height(canvas_light, height);
}

real_t Light2D::get_height() const { return height; }

void Light2D::set_energy(real_t p_energy)
{
	energy = p_energy;
	Renderer::canvas_light_set_energy(canvas_light, energy);
}

real_t Light2D::get_energy() const { return energy; }

void Light2D::set_z_range_min(int p_min_z)
{
	z_min = p_min_z;
	Renderer::canvas_light_set_z_range(canvas_light, z_min, z_max);
}

int Light2D::get_z_range_min() const { return z_min; }

void Light2D::set_z_range_max(int p_max_z)
{
	z_max = p_max_z;
	Renderer::canvas_light_set_z_range(canvas_light, z_min, z_max);
}

int Light2D::get_z_range_max() const { return z_max; }

void Light2D::set_layer_range_min(int p_min_layer)
{
	layer_min = p_min_layer;
	Renderer::canvas_light_set_layer_range(canvas_light, layer_min, layer_max);
}

int Light2D::get_layer_range_min() const { return layer_min; }

void Light2D::set_layer_range_max(int p_max_layer)
{
	layer_max = p_max_layer;
	Renderer::canvas_light_set_layer_range(canvas_light, layer_min, layer_max);
}

int Light2D::get_layer_range_max() const { return layer_max; }

void Light2D::set_item_cull_mask(int p_mask)
{
	item_mask = p_mask;
	Renderer::canvas_light_set_item_cull_mask(canvas_light, item_mask);
}

int Light2D::get_item_cull_mask() const { return item_mask; }

void Light2D::set_item_shadow_cull_mask(int p_mask)
{
	item_shadow_mask = p_mask;
	Renderer::canvas_light_set_item_shadow_cull_mask(canvas_light, item_shadow_mask);
}

int Light2D::get_item_shadow_cull_mask() const { return item_shadow_mask; }

void Light2D::set_shadow_enabled(bool p_enabled)
{
	shadow = p_enabled;
	Renderer::canvas_light_set_shadow_enabled(canvas_light, shadow);
}

bool Light2D::is_shadow_enabled() const { return shadow; }

void Light2D::set_shadow_filter(ShadowFilter p_filter)
{
	ERR_FAIL_INDEX(p_filter, SHADOW_FILTER_MAX);
	shadow_filter = p_filter;
	Renderer::canvas_light_set_shadow_filter(
		canvas_light, RSE::CanvasLightShadowFilter(p_filter));
}

Light2D::ShadowFilter Light2D::get_shadow_filter() const { return shadow_filter; }

void Light2D::set_shadow_color(const Color& p_shadow_color)
{
	shadow_color = p_shadow_color;
	Renderer::canvas_light_set_shadow_color(canvas_light, shadow_color);
}

Color Light2D::get_shadow_color() const { return shadow_color; }

void Light2D::set_blend_mode(BlendMode p_mode)
{
	blend_mode = p_mode;
	Renderer::canvas_light_set_blend_mode(canvas_light, RSE::CanvasLightBlendMode(p_mode));
}

Light2D::BlendMode Light2D::get_blend_mode() const { return blend_mode; }

void Light2D::_physics_interpolated_changed()
{
	Renderer::canvas_light_set_interpolated(canvas_light, is_physics_interpolated());
}

void Light2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_CANVAS: {
		Renderer::canvas_light_attach_to_canvas(canvas_light, get_canvas());
		_update_light_visibility();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		Renderer::canvas_light_set_transform(canvas_light, get_global_transform());
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		_update_light_visibility();
	} break;

	case NOTIFICATION_RESET_PHYSICS_INTERPOLATION: {
		if (is_visible_in_tree() && is_physics_interpolated_and_enabled()) {
			Renderer::canvas_light_set_transform(canvas_light, get_global_transform());
			Renderer::canvas_light_reset_physics_interpolation(canvas_light);
		}
	} break;

	case NOTIFICATION_EXIT_CANVAS: {
		Renderer::canvas_light_attach_to_canvas(canvas_light, RID());
		_update_light_visibility();
	} break;
	}
}

void Light2D::set_shadow_smooth(real_t p_amount)
{
	shadow_smooth = p_amount;
	Renderer::canvas_light_set_shadow_smooth(canvas_light, shadow_smooth);
}

real_t Light2D::get_shadow_smooth() const { return shadow_smooth; }

#ifdef TOOLS_ENABLED
void Light2D::_edit_set_pivot(const Point2& p_pivot)
{
	if (light_type == LIGHT_POINT) {
		set_position(get_transform().xform(p_pivot));
		set_texture_offset(get_texture_offset() - p_pivot);
	}
}

Point2 Light2D::_edit_get_pivot() const { return Vector2(); }

bool Light2D::_edit_use_pivot() const { return light_type == LIGHT_POINT; }
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
Rect2 Light2D::_edit_get_rect() const
{
	if (light_type != LIGHT_POINT || texture.is_null()) {
		return Rect2();
	}
	Size2 s = texture->get_size() * _scale;
	return Rect2(texture_offset - s / 2.0, s);
}

bool Light2D::_edit_use_rect() const { return light_type == LIGHT_POINT && texture.is_valid(); }
#endif // DEBUG_ENABLED

Rect2 Light2D::get_anchorable_rect() const
{
	if (light_type != LIGHT_POINT || texture.is_null()) {
		return Rect2();
	}
	Size2 s = texture->get_size() * _scale;
	return Rect2(texture_offset - s / 2.0, s);
}

void Light2D::set_texture(const Ref<Texture2D>& p_texture)
{
	texture = p_texture;
	if (canvas_light.is_valid() && Renderer::data) {
		Renderer::canvas_light_set_texture(
			canvas_light, texture.is_valid() ? texture->get_rid() : RID());
	}
	item_rect_changed();
}

Ref<Texture2D> Light2D::get_texture() const { return texture; }

void Light2D::set_texture_offset(const Vector2& p_offset)
{
	texture_offset = p_offset;
	Renderer::canvas_light_set_texture_offset(canvas_light, texture_offset);
	item_rect_changed();
}

Vector2 Light2D::get_texture_offset() const { return texture_offset; }

void Light2D::set_texture_scale(real_t p_scale)
{
	_scale = p_scale;
	if (_scale == 0) {
		_scale = CMP_EPSILON;
	}
	Renderer::canvas_light_set_texture_scale(canvas_light, _scale);
	item_rect_changed();
}

real_t Light2D::get_texture_scale() const { return _scale; }

void Light2D::set_max_distance(real_t p_distance)
{
	max_distance = p_distance;
	if (light_type == LIGHT_DIRECTIONAL) {
		Renderer::canvas_light_set_directional_distance(canvas_light, max_distance);
	}
}

real_t Light2D::get_max_distance() const { return max_distance; }

PackedStringArray Light2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (light_type == LIGHT_POINT && texture.is_null()) {
		warnings.push_back(RTR(
			"A texture with the shape of the light must be supplied to the \"Texture\" property."));
	}

	return warnings;
}

Light2D::Light2D(LightType p_type) : light_type(p_type)
{
	if (Renderer::data) {
		canvas_light = Renderer::canvas_light_create();
		set_light_type(p_type);
	}
	set_notify_transform(true);
	set_hide_clip_children(true);
}

Light2D::~Light2D()
{
	if (canvas_light.is_valid() && Renderer::data) {
		Renderer::free_rid(canvas_light);
	}
}
