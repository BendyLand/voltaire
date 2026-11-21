/**************************************************************************/
/*  openxr_scene.cpp                                                      */
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

#include "../extensions/openxr_extensions.h"
#include "../openxr_api.h"
#include "../openxr_interface.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "openxr_scene.h"
#include "platform/android/api/java_class_wrapper.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/xr/xr_nodes.h"
#include "scene/main/viewport.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_server.h"

////////////////////////////////////////////////////////////////////////////
// OpenXRCompositionLayer

Vector<OpenXRCompositionLayer*> OpenXRCompositionLayer::composition_layer_nodes;

static const char* HOLE_PUNCH_SHADER_CODE =
	"shader_type spatial;\n"
	"render_mode blend_mix, depth_draw_opaque, cull_back, shadow_to_opacity, shadows_disabled;\n"
	"void fragment() {\n"
	"\tALBEDO = vec3(0.0, 0.0, 0.0);\n"
	"}\n";

OpenXRCompositionLayer::OpenXRCompositionLayer(LayerType p_type) : layer_type(p_type)
{
	if (!OpenXRCompositionLayerExtension::is_initialized()) {
		return;
	}

	switch (layer_type) {
	case LAYER_QUAD: {
		XrCompositionLayerQuad openxr_composition_layer = {
			XR_TYPE_COMPOSITION_LAYER_QUAD,
			nullptr,
			0,
			XR_NULL_HANDLE,
			XR_EYE_VISIBILITY_BOTH,
			{},
			{{0, 0, 0, 0}, {0, 0, 0}},
			{(float)quad_size.x, (float)quad_size.y},
		};
		composition_layer = OpenXRCompositionLayerExtension::composition_layer_create(
			(XrCompositionLayerBaseHeader*)&openxr_composition_layer);
	} break;
	case LAYER_CYLINDER: {
		XrCompositionLayerCylinderKHR openxr_composition_layer = {
			XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR,
			nullptr,
			0,
			XR_NULL_HANDLE,
			XR_EYE_VISIBILITY_BOTH,
			{},
			{{0, 0, 0, 0}, {0, 0, 0}},
			radius,
			central_angle,
			aspect_ratio,
		};
		composition_layer = OpenXRCompositionLayerExtension::composition_layer_create(
			(XrCompositionLayerBaseHeader*)&openxr_composition_layer);
	} break;
	case LAYER_EQUIRECT: {
		XrCompositionLayerEquirect2KHR openxr_composition_layer = {
			XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR,
			nullptr,
			0,
			XR_NULL_HANDLE,
			XR_EYE_VISIBILITY_BOTH,
			{},
			{{0, 0, 0, 0}, {0, 0, 0}},
			radius,
			central_horizontal_angle,
			upper_vertical_angle,
			-lower_vertical_angle,
		};
		composition_layer = OpenXRCompositionLayerExtension::composition_layer_create(
			(XrCompositionLayerBaseHeader*)&openxr_composition_layer);
	} break;
	}
}

XrStructureType OpenXRCompositionLayer::_get_openxr_type() const
{
	switch (layer_type) {
	case LAYER_QUAD:
		return XR_TYPE_COMPOSITION_LAYER_QUAD;
	case LAYER_CYLINDER:
		return XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR;
	case LAYER_EQUIRECT:
		return XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR;
	}
	return XR_TYPE_COMPOSITION_LAYER_QUAD;
}

Ref<Mesh> OpenXRCompositionLayer::_create_fallback_mesh()
{
	if (layer_type == LAYER_QUAD) {
		Ref<PrimitiveMesh> mesh;
		mesh.instantiate(PrimitiveMesh::MESH_QUAD);
		mesh->set_plane_size(quad_size);
		return mesh;
	}
	Ref<Mesh> m = memnew(Mesh);
	return m;
}

bool OpenXRCompositionLayer::_should_use_fallback_node()
{
	if (Engine::is_editor_hint() || !OpenXRAPI::is_allocated()) {
		return true;
	}
	else if (openxr_session_running) {
		return enable_hole_punch || !is_natively_supported();
	}
	return false;
}

void OpenXRCompositionLayer::_create_fallback_node()
{
	ERR_FAIL_COND(fallback);
	fallback = memnew(MeshInstance3D);
	fallback->set_cast_shadows_setting(RenderInstance3D::SHADOW_CASTING_SETTING_OFF);
	add_child(fallback, false, INTERNAL_MODE_FRONT);
	should_update_fallback_mesh = true;
}

void OpenXRCompositionLayer::_remove_fallback_node()
{
	ERR_FAIL_COND(fallback == nullptr);
	remove_child(fallback);
	fallback->queue_free();
	fallback = nullptr;
}

void OpenXRCompositionLayer::_setup_composition_layer()
{
	if (use_android_surface || layer_viewport) {
		if (OpenXRCompositionLayerExtension::is_initialized()) {
			OpenXRCompositionLayerExtension::composition_layer_register(composition_layer);
			registered = true;

			if (layer_viewport) {
				OpenXRCompositionLayerExtension::composition_layer_set_viewport(composition_layer,
					layer_viewport->get_viewport_rid(), layer_viewport->get_size());
			}
		}
	}
}

void OpenXRCompositionLayer::_clear_composition_layer()
{
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_unregister(composition_layer);
		registered = false;

		if (!use_android_surface) {
			OpenXRCompositionLayerExtension::composition_layer_set_viewport(
				composition_layer, RID(), Size2i());
		}
	}
}

void OpenXRCompositionLayer::_viewport_size_changed()
{
	if (layer_viewport && openxr_session_running &&
		OpenXRCompositionLayerExtension::is_initialized() && is_natively_supported() &&
		is_visible_in_tree() && is_inside_tree()) {
		OpenXRCompositionLayerExtension::composition_layer_set_viewport(
			composition_layer, layer_viewport->get_viewport_rid(), layer_viewport->get_size());
	}
}

void OpenXRCompositionLayer::_on_openxr_session_begun()
{
	openxr_session_running = true;
	if (_should_register()) {
		_setup_composition_layer();
	}
	if (!fallback && _should_use_fallback_node()) {
		_create_fallback_node();
	}
}

void OpenXRCompositionLayer::_on_openxr_session_stopping()
{
	openxr_session_running = false;
	if (fallback && !_should_use_fallback_node()) {
		_remove_fallback_node();
	}
	_clear_composition_layer();
}

void OpenXRCompositionLayer::update_fallback_mesh() { should_update_fallback_mesh = true; }

bool OpenXRCompositionLayer::_should_register()
{
	return !registered && openxr_session_running && is_inside_tree() && is_visible_in_tree() &&
		   is_natively_supported();
}

bool OpenXRCompositionLayer::is_viewport_in_use(SubViewport* p_viewport)
{
	ERR_FAIL_NULL_V(p_viewport, false);
	for (const OpenXRCompositionLayer* other_composition_layer : composition_layer_nodes) {
		if (other_composition_layer != this && other_composition_layer->is_inside_tree() &&
			other_composition_layer->get_layer_viewport() == p_viewport) {
			return true;
		}
	}
	return false;
}

void OpenXRCompositionLayer::set_use_android_surface(bool p_use_android_surface)
{
	if (use_android_surface == p_use_android_surface) {
		return;
	}

	use_android_surface = p_use_android_surface;
	if (use_android_surface) {
		set_layer_viewport(nullptr);
		if (OpenXRCompositionLayerExtension::is_initialized()) {
			OpenXRCompositionLayerExtension::composition_layer_set_use_android_surface(
				composition_layer, true, android_surface_size);
		}
		if (_should_register()) {
			_setup_composition_layer();
		}
	}
	else {
		if (OpenXRCompositionLayerExtension::is_initialized()) {
			OpenXRCompositionLayerExtension::composition_layer_set_use_android_surface(
				composition_layer, false, Size2i());
		}
	}
}

bool OpenXRCompositionLayer::get_use_android_surface() const { return use_android_surface; }

void OpenXRCompositionLayer::set_android_surface_size(Size2i p_size)
{
	if (android_surface_size == p_size) {
		return;
	}

	android_surface_size = p_size;
	if (use_android_surface && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_use_android_surface(
			composition_layer, true, android_surface_size);
	}
}

Size2i OpenXRCompositionLayer::get_android_surface_size() const { return android_surface_size; }

SubViewport* OpenXRCompositionLayer::get_layer_viewport() const { return layer_viewport; }

bool OpenXRCompositionLayer::get_enable_hole_punch() const { return enable_hole_punch; }

int OpenXRCompositionLayer::get_sort_order() const { return sort_order; }

void OpenXRCompositionLayer::set_alpha_blend(bool p_alpha_blend)
{
	alpha_blend = p_alpha_blend;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_alpha_blend(
			composition_layer, p_alpha_blend);
	}
	if (fallback) {
		_reset_fallback_material();
	}
}

bool OpenXRCompositionLayer::get_alpha_blend() const { return alpha_blend; }

bool OpenXRCompositionLayer::is_natively_supported() const
{
	if (OpenXRCompositionLayerExtension::is_initialized() && OpenXRAPI::is_allocated()) {
		return OpenXRCompositionLayerExtension::is_available(_get_openxr_type());
	}
	return false;
}

void OpenXRCompositionLayer::set_protected_content(bool p_protected_content)
{
	if (protected_content == p_protected_content) {
		return;
	}
	protected_content = p_protected_content;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_protected_content(
			composition_layer, p_protected_content);
	}
}

bool OpenXRCompositionLayer::is_protected_content() const { return protected_content; }

void OpenXRCompositionLayer::set_min_filter(Filter p_mode)
{
	if (min_filter == p_mode) {
		return;
	}
	min_filter = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_min_filter(
			composition_layer, (OpenXRCompositionLayerExtension::Filter)p_mode);
	}
}

OpenXRCompositionLayer::Filter OpenXRCompositionLayer::get_min_filter() const { return min_filter; }

void OpenXRCompositionLayer::set_mag_filter(Filter p_mode)
{
	if (mag_filter == p_mode) {
		return;
	}
	mag_filter = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_mag_filter(
			composition_layer, (OpenXRCompositionLayerExtension::Filter)p_mode);
	}
}

OpenXRCompositionLayer::Filter OpenXRCompositionLayer::get_mag_filter() const { return mag_filter; }

void OpenXRCompositionLayer::set_mipmap_mode(MipmapMode p_mode)
{
	if (mipmap_mode == p_mode) {
		return;
	}
	mipmap_mode = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_mipmap_mode(
			composition_layer, (OpenXRCompositionLayerExtension::MipmapMode)p_mode);
	}
}

OpenXRCompositionLayer::MipmapMode OpenXRCompositionLayer::get_mipmap_mode() const
{
	return mipmap_mode;
}

void OpenXRCompositionLayer::set_horizontal_wrap(Wrap p_mode)
{
	if (horizontal_wrap == p_mode) {
		return;
	}
	horizontal_wrap = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_horizontal_wrap(
			composition_layer, (OpenXRCompositionLayerExtension::Wrap)p_mode);
	}
}

OpenXRCompositionLayer::Wrap OpenXRCompositionLayer::get_horizontal_wrap() const
{
	return horizontal_wrap;
}

void OpenXRCompositionLayer::set_vertical_wrap(Wrap p_mode)
{
	if (vertical_wrap == p_mode) {
		return;
	}
	vertical_wrap = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_vertical_wrap(
			composition_layer, (OpenXRCompositionLayerExtension::Wrap)p_mode);
	}
}

OpenXRCompositionLayer::Wrap OpenXRCompositionLayer::get_vertical_wrap() const
{
	return vertical_wrap;
}

void OpenXRCompositionLayer::set_red_swizzle(Swizzle p_mode)
{
	if (red_swizzle == p_mode) {
		return;
	}
	red_swizzle = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_red_swizzle(
			composition_layer, (OpenXRCompositionLayerExtension::Swizzle)p_mode);
	}
}

OpenXRCompositionLayer::Swizzle OpenXRCompositionLayer::get_red_swizzle() const
{
	return red_swizzle;
}

void OpenXRCompositionLayer::set_green_swizzle(Swizzle p_mode)
{
	if (green_swizzle == p_mode) {
		return;
	}
	green_swizzle = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_green_swizzle(
			composition_layer, (OpenXRCompositionLayerExtension::Swizzle)p_mode);
	}
}

OpenXRCompositionLayer::Swizzle OpenXRCompositionLayer::get_green_swizzle() const
{
	return green_swizzle;
}

void OpenXRCompositionLayer::set_blue_swizzle(Swizzle p_mode)
{
	if (blue_swizzle == p_mode) {
		return;
	}
	blue_swizzle = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_blue_swizzle(
			composition_layer, (OpenXRCompositionLayerExtension::Swizzle)p_mode);
	}
}

OpenXRCompositionLayer::Swizzle OpenXRCompositionLayer::get_blue_swizzle() const
{
	return blue_swizzle;
}

void OpenXRCompositionLayer::set_alpha_swizzle(Swizzle p_mode)
{
	if (alpha_swizzle == p_mode) {
		return;
	}
	alpha_swizzle = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_alpha_swizzle(
			composition_layer, (OpenXRCompositionLayerExtension::Swizzle)p_mode);
	}
}

OpenXRCompositionLayer::Swizzle OpenXRCompositionLayer::get_alpha_swizzle() const
{
	return alpha_swizzle;
}

void OpenXRCompositionLayer::set_max_anisotropy(float p_value)
{
	if (max_anisotropy == p_value) {
		return;
	}
	max_anisotropy = p_value;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_max_anisotropy(
			composition_layer, p_value);
	}
}

float OpenXRCompositionLayer::get_max_anisotropy() const { return max_anisotropy; }

void OpenXRCompositionLayer::set_border_color(const Color& p_color)
{
	if (border_color == p_color) {
		return;
	}
	border_color = p_color;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_border_color(
			composition_layer, p_color);
	}
}

Color OpenXRCompositionLayer::get_border_color() const { return border_color; }

void OpenXRCompositionLayer::set_eye_visibility(EyeVisibility p_eye_visibility)
{
	if (eye_visibility == p_eye_visibility) {
		return;
	}
	eye_visibility = p_eye_visibility;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_eye_visibility(
			composition_layer, (OpenXRCompositionLayerExtension::EyeVisibility)p_eye_visibility);
	}
}

OpenXRCompositionLayer::EyeVisibility OpenXRCompositionLayer::get_eye_visibility() const
{
	return eye_visibility;
}

void OpenXRCompositionLayer::set_quad_size(const Size2& p_size)
{
	quad_size = p_size;
	if (layer_type == LAYER_QUAD && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_quad_size(composition_layer, p_size);
	}
	update_fallback_mesh();
}

Size2 OpenXRCompositionLayer::get_quad_size() const { return quad_size; }

void OpenXRCompositionLayer::set_radius(float p_radius)
{
	ERR_FAIL_COND(p_radius <= 0);
	radius = p_radius;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		if (layer_type == LAYER_CYLINDER) {
			OpenXRCompositionLayerExtension::composition_layer_set_cylinder_radius(
				composition_layer, p_radius);
		}
		else if (layer_type == LAYER_EQUIRECT) {
			OpenXRCompositionLayerExtension::composition_layer_set_equirect_radius(
				composition_layer, p_radius);
		}
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_radius() const { return radius; }

void OpenXRCompositionLayer::set_fallback_segments(uint32_t p_fallback_segments)
{
	ERR_FAIL_COND(p_fallback_segments == 0);
	fallback_segments = p_fallback_segments;
	update_fallback_mesh();
}

uint32_t OpenXRCompositionLayer::get_fallback_segments() const { return fallback_segments; }

void OpenXRCompositionLayer::set_aspect_ratio(float p_aspect_ratio)
{
	ERR_FAIL_COND(p_aspect_ratio <= 0);
	aspect_ratio = p_aspect_ratio;
	if (layer_type == LAYER_CYLINDER && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_cylinder_aspect_ratio(
			composition_layer, p_aspect_ratio);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_aspect_ratio() const { return aspect_ratio; }

void OpenXRCompositionLayer::set_central_angle(float p_central_angle)
{
	ERR_FAIL_COND(p_central_angle <= 0);
	central_angle = p_central_angle;
	if (layer_type == LAYER_CYLINDER && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_cylinder_central_angle(
			composition_layer, p_central_angle);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_central_angle() const { return central_angle; }

void OpenXRCompositionLayer::set_central_horizontal_angle(float p_angle)
{
	ERR_FAIL_COND(p_angle <= 0);
	central_horizontal_angle = p_angle;
	if (layer_type == LAYER_EQUIRECT && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_equirect_central_horizontal_angle(
			composition_layer, p_angle);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_central_horizontal_angle() const
{
	return central_horizontal_angle;
}

void OpenXRCompositionLayer::set_upper_vertical_angle(float p_angle)
{
	ERR_FAIL_COND(p_angle <= 0 || p_angle > (Math::PI / 2.0));
	upper_vertical_angle = p_angle;
	if (layer_type == LAYER_EQUIRECT && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_equirect_upper_vertical_angle(
			composition_layer, p_angle);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_upper_vertical_angle() const
{
	return upper_vertical_angle;
}

void OpenXRCompositionLayer::set_lower_vertical_angle(float p_angle)
{
	ERR_FAIL_COND(p_angle <= 0 || p_angle > (Math::PI / 2.0));
	lower_vertical_angle = p_angle;
	if (layer_type == LAYER_EQUIRECT && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_equirect_lower_vertical_angle(
			composition_layer, -p_angle);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_lower_vertical_angle() const
{
	return lower_vertical_angle;
}

Ref<JavaObject> OpenXRCompositionLayer::get_android_surface()
{
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		return OpenXRCompositionLayerExtension::composition_layer_get_android_surface(
			composition_layer);
	}
	return Ref<JavaObject>();
}

Vector2 OpenXRCompositionLayer::intersects_ray(
	const Vector3& p_origin, const Vector3& p_direction) const
{
	switch (layer_type) {
	case LAYER_QUAD: {
		Transform3D quad_transform = get_global_transform();
		Vector3 quad_normal = quad_transform.basis.get_column(2);

		float denom = quad_normal.dot(p_direction);
		if (Math::abs(denom) > 0.0001) {
			Vector3 vector = quad_transform.origin - p_origin;
			float t = vector.dot(quad_normal) / denom;
			if (t < 0.0) {
				return Vector2(-1.0, -1.0);
			}
			Vector3 intersection = p_origin + p_direction * t;

			Vector3 relative_point = intersection - quad_transform.origin;
			Vector2 projected_point = Vector2(relative_point.dot(quad_transform.basis.get_column(0)),
				relative_point.dot(quad_transform.basis.get_column(1)));
			if (Math::abs(projected_point.x) > quad_size.x / 2.0) {
				return Vector2(-1.0, -1.0);
			}
			if (Math::abs(projected_point.y) > quad_size.y / 2.0) {
				return Vector2(-1.0, -1.0);
			}

			float u = 0.5 + (projected_point.x / quad_size.x);
			float v = 1.0 - (0.5 + (projected_point.y / quad_size.y));

			return Vector2(u, v);
		}
		return Vector2(-1.0, -1.0);
	}
	case LAYER_CYLINDER: {
		Transform3D cylinder_transform = get_global_transform();
		Vector3 cylinder_axis = cylinder_transform.basis.get_column(1);

		Vector3 offset = p_origin - cylinder_transform.origin;
		float a = p_direction.dot(p_direction - cylinder_axis * p_direction.dot(cylinder_axis));
		float b = 2.0 * (p_direction.dot(offset - cylinder_axis * offset.dot(cylinder_axis)));
		float c =
			offset.dot(offset - cylinder_axis * offset.dot(cylinder_axis)) - (radius * radius);

		float discriminant = b * b - 4.0 * a * c;
		if (discriminant < 0.0) {
			return Vector2(-1.0, -1.0);
		}

		float t0 = (-b - Math::sqrt(discriminant)) / (2.0 * a);
		float t1 = (-b + Math::sqrt(discriminant)) / (2.0 * a);
		float t = MAX(t0, t1);

		if (t < 0.0) {
			return Vector2(-1.0, -1.0);
		}
		Vector3 intersection = p_origin + p_direction * t;

		Basis correction = cylinder_transform.basis.inverse();
		correction.rotate(Vector3(0.0, 1.0, 0.0), -Math::PI / 2.0);
		Vector3 relative_point = correction.xform(intersection - cylinder_transform.origin);

		Vector2 projected_point = Vector2(relative_point.x, relative_point.z);
		float intersection_angle = Math::atan2(projected_point.y, projected_point.x);
		if (Math::abs(intersection_angle) > central_angle / 2.0) {
			return Vector2(-1.0, -1.0);
		}

		float arc_length = radius * central_angle;
		float height = aspect_ratio * arc_length;
		if (Math::abs(relative_point.y) > height / 2.0) {
			return Vector2(-1.0, -1.0);
		}

		float u = 0.5 + (intersection_angle / central_angle);
		float v = 1.0 - (0.5 + (relative_point.y / height));

		return Vector2(u, v);
	}
	case LAYER_EQUIRECT: {
		Transform3D equirect_transform = get_global_transform();

		Vector3 offset = p_origin - equirect_transform.origin;
		float a = p_direction.dot(p_direction);
		float b = 2.0 * offset.dot(p_direction);
		float c = offset.dot(offset) - (radius * radius);

		float discriminant = b * b - 4.0 * a * c;
		if (discriminant < 0.0) {
			return Vector2(-1.0, -1.0);
		}

		float t0 = (-b - Math::sqrt(discriminant)) / (2.0 * a);
		float t1 = (-b + Math::sqrt(discriminant)) / (2.0 * a);
		float t = MAX(t0, t1);

		if (t < 0.0) {
			return Vector2(-1.0, -1.0);
		}
		Vector3 intersection = p_origin + p_direction * t;

		Basis correction = equirect_transform.basis.inverse();
		correction.rotate(Vector3(0.0, 1.0, 0.0), -Math::PI / 2.0);
		Vector3 relative_point = correction.xform(intersection - equirect_transform.origin);

		float horizontal_intersection_angle = Math::atan2(relative_point.z, relative_point.x);
		if (Math::abs(horizontal_intersection_angle) > central_horizontal_angle / 2.0) {
			return Vector2(-1.0, -1.0);
		}

		float vertical_intersection_angle =
			Math::acos(relative_point.y / radius) - (Math::PI / 2.0);
		if (vertical_intersection_angle < 0) {
			if (Math::abs(vertical_intersection_angle) > upper_vertical_angle) {
				return Vector2(-1.0, -1.0);
			}
		}
		else if (vertical_intersection_angle > lower_vertical_angle) {
			return Vector2(-1.0, -1.0);
		}

		if (upper_vertical_angle != lower_vertical_angle) {
			vertical_intersection_angle -= (-upper_vertical_angle + lower_vertical_angle) / 2.0;
		}

		float u = 0.5 + (horizontal_intersection_angle / central_horizontal_angle);
		float v = 0.5 + (vertical_intersection_angle / (upper_vertical_angle + lower_vertical_angle));

		return Vector2(u, v);
	}
	}
	return Vector2(-1.0, -1.0);
}

void OpenXRCompositionLayer::_reset_fallback_material()
{
	ERR_FAIL_NULL(fallback);

	if (fallback->get_mesh().is_null()) {
		return;
	}

	if (enable_hole_punch && !Engine::is_editor_hint() && is_natively_supported()) {
		Ref<ShaderMaterial> material = fallback->get_surface_override_material(0);
		if (material.is_null()) {
			Ref<Shader> shader;
			shader.instantiate();
			shader->set_code(HOLE_PUNCH_SHADER_CODE);

			material.instantiate();
			material->set_shader(shader);
			fallback->set_surface_override_material(0, material);
		}
	}
	else if (layer_viewport) {
		Ref<StandardMaterial3D> material = fallback->get_surface_override_material(0);
		if (material.is_null()) {
			material.instantiate();
			material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
			material->set_local_to_scene(true);
			fallback->set_surface_override_material(0, material);
		}
		material->set_transparency(get_alpha_blend() ? StandardMaterial3D::TRANSPARENCY_ALPHA
													 : StandardMaterial3D::TRANSPARENCY_DISABLED);
		material->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, layer_viewport->get_texture());
	}
	else {
		fallback->set_surface_override_material(0, Ref<Material>());
	}
}

PackedStringArray OpenXRCompositionLayer::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (!get_transform().basis.is_orthonormal()) {
		warnings.push_back(RTR("OpenXR composition layers must have orthonormalized transforms "
							   "(ie. no scale or shearing)."));
	}

	if (enable_hole_punch && get_sort_order() >= 0) {
		warnings.push_back(
			RTR("Hole punching won't work as expected unless the sort order is less than zero."));
	}

	return warnings;
}

Camera3D* OpenXRCompositionLayer::_get_xrcamera3d_ancestor() const
{
	Node* parent = get_parent();
	while (parent != nullptr) {
		Camera3D* camera = dynamic_cast<Camera3D*>(parent);
		if (camera != nullptr && camera->is_xr_enabled()) {
			return camera;
		}
		parent = parent->get_parent();
	}
	return nullptr;
}

void OpenXRCompositionLayer::set_layer_viewport(SubViewport* p_viewport) {}

////////////////////////////////////////////////////////////////////////////
// OpenXRHand

OpenXRHand::OpenXRHand() {}

void OpenXRHand::set_hand(Hands p_hand)
{
	ERR_FAIL_INDEX(p_hand, HAND_MAX);

	hand = p_hand;
}

OpenXRHand::Hands OpenXRHand::get_hand() const { return hand; }

void OpenXRHand::set_hand_skeleton(const NodePath& p_hand_skeleton)
{
	hand_skeleton = p_hand_skeleton;
}

void OpenXRHand::set_motion_range(MotionRange p_motion_range)
{
	ERR_FAIL_INDEX(p_motion_range, MOTION_RANGE_MAX);
	motion_range = p_motion_range;

	_set_motion_range();
}

OpenXRHand::MotionRange OpenXRHand::get_motion_range() const { return motion_range; }

NodePath OpenXRHand::get_hand_skeleton() const { return hand_skeleton; }

void OpenXRHand::set_skeleton_rig(SkeletonRig p_skeleton_rig)
{
	ERR_FAIL_INDEX(p_skeleton_rig, SKELETON_RIG_MAX);

	skeleton_rig = p_skeleton_rig;
}

OpenXRHand::SkeletonRig OpenXRHand::get_skeleton_rig() const { return skeleton_rig; }

void OpenXRHand::set_bone_update(BoneUpdate p_bone_update)
{
	ERR_FAIL_INDEX(p_bone_update, BONE_UPDATE_MAX);

	bone_update = p_bone_update;
}

OpenXRHand::BoneUpdate OpenXRHand::get_bone_update() const { return bone_update; }

void OpenXRHand::_get_joint_data()
{
	static const String bone_names[SKELETON_RIG_MAX][XR_HAND_JOINT_COUNT_EXT] = {
		{"Palm", "Wrist", "Thumb_Metacarpal", "Thumb_Proximal", "Thumb_Distal", "Thumb_Tip",
			"Index_Metacarpal", "Index_Proximal", "Index_Intermediate", "Index_Distal", "Index_Tip",
			"Middle_Metacarpal", "Middle_Proximal", "Middle_Intermediate", "Middle_Distal",
			"Middle_Tip", "Ring_Metacarpal", "Ring_Proximal", "Ring_Intermediate", "Ring_Distal",
			"Ring_Tip", "Little_Metacarpal", "Little_Proximal", "Little_Intermediate",
			"Little_Distal", "Little_Tip"},
		{"Palm", "Hand", "ThumbMetacarpal", "ThumbProximal", "ThumbDistal", "ThumbTip",
			"IndexMetacarpal", "IndexProximal", "IndexIntermediate", "IndexDistal", "IndexTip",
			"MiddleMetacarpal", "MiddleProximal", "MiddleIntermediate", "MiddleDistal", "MiddleTip",
			"RingMetacarpal", "RingProximal", "RingIntermediate", "RingDistal", "RingTip",
			"LittleMetacarpal", "LittleProximal", "LittleIntermediate", "LittleDistal",
			"LittleTip"}};

	static const String bone_name_formats[SKELETON_RIG_MAX][2] = {
		{"<bone>_L", "<bone>_R"},
		{"Left<bone>", "Right<bone>"}};

	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		joints[i].bone = -1;
		joints[i].parent_joint = -1;
	}

	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}

	int bones[XR_HAND_JOINT_COUNT_EXT];
	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		String bone_name =
			bone_name_formats[skeleton_rig][hand].replace("<bone>", bone_names[skeleton_rig][i]);
		bones[i] = skeleton->find_bone(bone_name);
	}

	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		const int bone = bones[i];
		if (bone == -1) {
			continue;
		}

		const int parent_bone = skeleton->get_bone_parent(bone);
		if (parent_bone == -1) {
			joints[i].bone = bone;
			joints[i].parent_joint = XR_HAND_JOINT_PALM_EXT;
			continue;
		}

		for (int j = 0; j < XR_HAND_JOINT_COUNT_EXT; ++j) {
			if (bones[j] == parent_bone) {
				joints[i].bone = bone;
				joints[i].parent_joint = j;
				break;
			}
		}
	}
}

void OpenXRHand::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_get_joint_data();
		set_process_internal(true);
	} break;
	case NOTIFICATION_EXIT_TREE: {
		set_process_internal(false);
		for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
			joints[i].bone = -1;
			joints[i].parent_joint = -1;
		}
	} break;
	case NOTIFICATION_INTERNAL_PROCESS: {
		_update_skeleton();
	} break;
	default: {
	} break;
	}
}

void OpenXRHand::_update_skeleton() {}

void OpenXRHand::_set_motion_range() {}

Skeleton3D* OpenXRHand::get_skeleton() { return nullptr; }

////////////////////////////////////////////////////////////////////////////
// OpenXRVisibilityMask

void OpenXRVisibilityMask::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void OpenXRVisibilityMask::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		if (OpenXRVisibilityMaskExtension::is_initialized() &&
			OpenXRVisibilityMaskExtension::is_available()) {
			set_base(OpenXRVisibilityMaskExtension::get_mesh());
		}
	} break;
	case NOTIFICATION_EXIT_TREE: {
		set_base(RID());
	} break;
	}
}

void OpenXRVisibilityMask::_on_openxr_session_begun()
{
	if (is_inside_tree()) {
		if (OpenXRVisibilityMaskExtension::is_initialized() &&
			OpenXRVisibilityMaskExtension::is_available()) {
			set_base(OpenXRVisibilityMaskExtension::get_mesh());
		}
	}
}

void OpenXRVisibilityMask::_on_openxr_session_stopping() { set_base(RID()); }

AABB OpenXRVisibilityMask::get_aabb() const
{
	AABB ret;
	ret.position = Vector3(-1000.0, -1000.0, -1000.0);
	ret.size = Vector3(2000.0, 2000.0, 2000.0);
	return ret;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRRenderModel & OpenXRRenderModelManager

#ifdef MODULE_GLTF_ENABLED

void OpenXRRenderModel::_load_render_model_scene() {}

String OpenXRRenderModel::get_top_level_path() const
{
	String ret;

	if (render_model.is_valid() && OpenXRRenderModelExtension::is_initialized()) {
		ret = OpenXRRenderModelExtension::render_model_get_top_level_path_as_string(render_model);
	}

	return ret;
}

RID OpenXRRenderModel::get_render_model() const { return render_model; }

void OpenXRRenderModel::set_render_model(RID p_render_model)
{
	render_model = p_render_model;
	if (is_inside_tree() && render_model.is_valid()) {
		_load_render_model_scene();
	}
}

bool OpenXRRenderModelManager::_has_filters() { return tracker != 0; }

void OpenXRRenderModelManager::_on_render_model_added(RID p_render_model)
{
	if (_has_filters()) {
		is_dirty = true;
	}
	else {
		_update_models();
	}
}

void OpenXRRenderModelManager::_on_render_model_removed(RID p_render_model)
{
	if (_has_filters()) {
		is_dirty = true;
	}
	else {
		_update_models();
	}
}

void OpenXRRenderModelManager::_on_render_model_top_level_path_changed(RID p_path)
{
	if (_has_filters()) {
		is_dirty = true;
	}
}

void OpenXRRenderModelManager::set_tracker(RenderModelTracker p_tracker)
{
	if (tracker != p_tracker) {
		tracker = p_tracker;
		is_dirty = true;

		if (tracker == RENDER_MODEL_TRACKER_ANY || tracker == RENDER_MODEL_TRACKER_NONE_SET) {
			xr_path = XR_NULL_PATH;
		}
		else if (!Engine::is_editor_hint()) {
			if (OpenXRAPI::is_allocated() && XRServer::is_initialized()) {
				String toplevel_path;
				String tracker_name;
				if (tracker == RENDER_MODEL_TRACKER_LEFT_HAND) {
					tracker_name = "left_hand";
					toplevel_path = "/user/hand/left";
				}
				else if (tracker == RENDER_MODEL_TRACKER_RIGHT_HAND) {
					tracker_name = "right_hand";
					toplevel_path = "/user/hand/right";
				}
				else {
					ERR_FAIL_MSG("Unsupported tracker value set.");
				}

				positional_tracker = XRServer::get_tracker(tracker_name);
				if (positional_tracker.is_null()) {
					WARN_PRINT("OpenXR: Can't find tracker " + tracker_name);
				}

				xr_path = OpenXRAPI::get_xr_path(toplevel_path);
				if (xr_path == XR_NULL_PATH) {
					WARN_PRINT("OpenXR: Can't find path for " + toplevel_path);
				}
			}
		}

		set_process_internal(true);
	}
}

OpenXRRenderModelManager::RenderModelTracker OpenXRRenderModelManager::get_tracker() const
{
	return tracker;
}

void OpenXRRenderModelManager::set_make_local_to_pose(const String& p_action)
{
	if (make_local_to_pose != p_action) {
		make_local_to_pose = p_action;

		if (container) {
			container->set_transform(Transform3D());
		}
	}
}

String OpenXRRenderModelManager::get_make_local_to_pose() const { return make_local_to_pose; }

void OpenXRRenderModelManager::_update_models() {}

#endif // MODULE_GLTF_ENABLED
