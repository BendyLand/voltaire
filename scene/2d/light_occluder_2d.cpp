/**************************************************************************/
/*  light_occluder_2d.cpp                                                 */
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
#include "light_occluder_2d.h"
#include "servers/rendering/rendering_server.h"

#define LINE_GRAB_WIDTH 8

#ifdef DEBUG_ENABLED
Rect2 OccluderPolygon2D::_edit_get_rect() const
{
	if (rect_cache_dirty) {
		if (closed) {
			const Vector2* r = polygon.ptr();
			item_rect = Rect2();
			for (int i = 0; i < polygon.size(); i++) {
				Vector2 pos = r[i];
				if (i == 0) {
					item_rect.position = pos;
				}
				else {
					item_rect.expand_to(pos);
				}
			}
			rect_cache_dirty = false;
		}
		else {
			if (polygon.is_empty()) {
				item_rect = Rect2();
			}
			else {
				Vector2 d = Vector2(LINE_GRAB_WIDTH, LINE_GRAB_WIDTH);
				item_rect = Rect2(polygon[0] - d, 2 * d);
				for (int i = 1; i < polygon.size(); i++) {
					item_rect.expand_to(polygon[i] - d);
					item_rect.expand_to(polygon[i] + d);
				}
			}
		}
	}

	return item_rect;
}

#endif // DEBUG_ENABLED

void OccluderPolygon2D::set_polygon(const Vector<Vector2>& p_polygon)
{
	polygon = p_polygon;
	rect_cache_dirty = true;
	RS::get_singleton()->canvas_occluder_polygon_set_shape(occ_polygon, p_polygon, closed);
	emit_changed();
	update_configuration_warning();
}

Vector<Vector2> OccluderPolygon2D::get_polygon() const { return polygon; }

void OccluderPolygon2D::set_closed(bool p_closed)
{
	if (closed == p_closed) {
		return;
	}
	closed = p_closed;
	if (polygon.size()) {
		RS::get_singleton()->canvas_occluder_polygon_set_shape(occ_polygon, polygon, closed);
	}
	emit_changed();
}

bool OccluderPolygon2D::is_closed() const { return closed; }

void OccluderPolygon2D::set_cull_mode(CullMode p_mode)
{
	cull = p_mode;
	RS::get_singleton()->canvas_occluder_polygon_set_cull_mode(
		occ_polygon, RSE::CanvasOccluderPolygonCullMode(p_mode));
}

OccluderPolygon2D::CullMode OccluderPolygon2D::get_cull_mode() const { return cull; }

RID OccluderPolygon2D::get_rid() const { return occ_polygon; }

OccluderPolygon2D::OccluderPolygon2D()
{
	occ_polygon = RS::get_singleton()->canvas_occluder_polygon_create();
}

OccluderPolygon2D::~OccluderPolygon2D()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(occ_polygon);
}

void LightOccluder2D::_physics_interpolated_changed()
{
	RenderingServer::get_singleton()->canvas_light_occluder_set_interpolated(
		occluder, is_physics_interpolated());
}

#ifdef DEBUG_ENABLED
Rect2 LightOccluder2D::_edit_get_rect() const
{
	return occluder_polygon.is_valid() ? occluder_polygon->_edit_get_rect() : Rect2();
}

bool LightOccluder2D::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	return occluder_polygon.is_valid()
			   ? occluder_polygon->_edit_is_selected_on_click(p_point, p_tolerance)
			   : false;
}
#endif // DEBUG_ENABLED

Ref<OccluderPolygon2D> LightOccluder2D::get_occluder_polygon() const { return occluder_polygon; }

void LightOccluder2D::set_occluder_light_mask(int p_mask)
{
	mask = p_mask;
	RS::get_singleton()->canvas_light_occluder_set_light_mask(occluder, p_mask);
}

int LightOccluder2D::get_occluder_light_mask() const { return mask; }

PackedStringArray LightOccluder2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (occluder_polygon.is_null()) {
		warnings.push_back(
			RTR("An occluder polygon must be set (or drawn) for this occluder to take effect."));
	}

	if (occluder_polygon.is_valid() && occluder_polygon->get_polygon().is_empty()) {
		warnings.push_back(
			RTR("The occluder polygon for this occluder is empty. Please draw a polygon."));
	}

	return warnings;
}

void LightOccluder2D::set_as_sdf_collision(bool p_enable)
{
	sdf_collision = p_enable;
	RS::get_singleton()->canvas_light_occluder_set_as_sdf_collision(occluder, sdf_collision);
}

bool LightOccluder2D::is_set_as_sdf_collision() const { return sdf_collision; }

LightOccluder2D::LightOccluder2D()
{
	occluder = RS::get_singleton()->canvas_light_occluder_create();

	set_notify_transform(true);
	set_as_sdf_collision(true);
}

LightOccluder2D::~LightOccluder2D()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());

	RS::get_singleton()->free_rid(occluder);
}


