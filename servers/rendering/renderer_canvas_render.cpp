/**************************************************************************/
/*  renderer_canvas_render.cpp                                            */
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

#include "renderer_canvas_render.h"
#include "servers/rendering/renderer.h"

const Rect2& RCR::Item::get_rect() const
{
	if (custom_rect || (!rect_dirty && !update_when_visible && skeleton == RID())) {
		return rect;
	}

	// must update rect

	if (commands == nullptr) {
		rect = Rect2();
		rect_dirty = false;
		return rect;
	}

	Transform2D xf;
	bool found_xform = false;
	bool first = true;

	const Item::Command* c = commands;

	while (c) {
		Rect2 r;

		switch (c->type) {
		case Item::Command::TYPE_RECT: {
			const Item::CommandRect* crect = static_cast<const Item::CommandRect*>(c);
			r = crect->rect;

		} break;
		case Item::Command::TYPE_NINEPATCH: {
			const Item::CommandNinePatch* style = static_cast<const Item::CommandNinePatch*>(c);
			r = style->rect;
		} break;

		case Item::Command::TYPE_POLYGON: {
			const Item::CommandPolygon* polygon = static_cast<const Item::CommandPolygon*>(c);
			r = polygon->polygon.rect_cache;
		} break;
		case Item::Command::TYPE_PRIMITIVE: {
			const Item::CommandPrimitive* primitive = static_cast<const Item::CommandPrimitive*>(c);
			for (uint32_t j = 0; j < primitive->point_count; j++) {
				if (j == 0) {
					r.position = primitive->points[0];
				}
				else {
					r.expand_to(primitive->points[j]);
				}
			}
		} break;
		case Item::Command::TYPE_MESH: {
			const Item::CommandMesh* mesh = static_cast<const Item::CommandMesh*>(c);
			AABB aabb = RendererMeshStorage::mesh_get_aabb(mesh->mesh, skeleton);

			r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);

		} break;
		case Item::Command::TYPE_MULTIMESH: {
			const Item::CommandMultiMesh* multimesh = static_cast<const Item::CommandMultiMesh*>(c);
			AABB aabb = RendererMeshStorage::multimesh_get_aabb(multimesh->multimesh);

			r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);

		} break
;
		case Item::Command::TYPE_PARTICLES: {
			const Item::CommandParticles* particles_cmd =
				static_cast<const Item::CommandParticles*>(c);
			if (particles_cmd->particles.is_valid()) {
				AABB aabb = RS::particles_storage->particles_get_aabb(particles_cmd->particles);
				r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);
			}

		} break;
		case Item::Command::TYPE_TRANSFORM: {
			const Item::CommandTransform* transform = static_cast<const Item::CommandTransform*>(c);
			xf = transform->xform;
			found_xform = true;
			[[fallthrough]];
		}
		default: {
			c = c->next;
			continue;
		}
		}

		if (found_xform) {
			r = xf.xform(r);
		}

		if (first) {
			rect = r;
			first = false;
		}
		else {
			rect = rect.merge(r);
		}
		c = c->next;
	}

	rect_dirty = false;
	return rect;
}

RCR::Item::CommandMesh::~CommandMesh()
{
	if (mesh_instance.is_valid()) {
		RendererMeshStorage::mesh_instance_free(mesh_instance);
	}
}


void Polygon::create(const Vector<int>& p_indices, const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, const Vector<Point2>& p_uvs,
	const Vector<int>& p_bones,
	const Vector<float>& p_weights, int p_count)
{
	ERR_FAIL_COND(polygon_id != 0);
	int count = p_count < 0 ? p_indices.size() : p_count * 3;
	ERR_FAIL_COND(count > p_indices.size());
	{
		uint32_t pc = p_points.size();
		const Vector2* v2 = p_points.ptr();
		rect_cache.position = *v2;
		for (uint32_t i = 1; i < pc; i++) {
			rect_cache.expand_to(v2[i]);
		}
	}
	polygon_id = RendererCanvasRender::request_polygon(
		p_indices, p_points, p_colors, p_uvs, p_bones, p_weights, count);
}

Polygon::~Polygon()
{
	if (polygon_id && RendererCanvasRender::free_polygon) {
		RendererCanvasRender::free_polygon(polygon_id);
	}
}
