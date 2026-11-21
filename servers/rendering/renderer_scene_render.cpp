/**************************************************************************/
/*  renderer_scene_render.cpp                                             */
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

#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/display/display_server.h"
#include "core/os/os.h"

#if defined(RD_ENABLED)
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#endif

#if defined(GLES3_ENABLED)
#include "drivers/gles3/rasterizer_gles3.h"
#endif

#include "servers/rendering/dummy/rasterizer_dummy.h"

#ifndef XR_DISABLED
#include "core/config/project_settings.h"
#include "servers/xr/xr_server.h"
#endif // XR_DISABLED
#include "servers/rendering/renderer_canvas_render.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_geometry_instance.h"

// --- Merged from servers/rendering/renderer_scene_render.cpp ---

void RendererSceneRender::CameraData::set_camera(const Transform3D p_transform,
	const Projection p_projection, bool p_is_orthogonal, bool p_vaspect,
	const Vector2& p_taa_jitter, float p_taa_frame_count, uint32_t p_visible_layers)
{
	view_count = 1;
	is_orthogonal = p_is_orthogonal;
	vaspect = p_vaspect;

	main_transform = p_transform;
	main_projection = p_projection;

	visible_layers = p_visible_layers;
	view_offset[0] = Transform3D();
	view_projection[0] = p_projection;
	taa_jitter = p_taa_jitter;
	taa_frame_count = p_taa_frame_count;
}

void RendererSceneRender::CameraData::set_multiview_camera(uint32_t p_view_count,
	const Transform3D* p_transforms, const Projection* p_projections, bool p_is_orthogonal,
	bool p_vaspect, uint32_t p_visible_layers)
{
	ERR_FAIL_COND_MSG(p_view_count != 2, "Incorrect view count for stereoscopic view");

	visible_layers = p_visible_layers;
	view_count = p_view_count;
	is_orthogonal = p_is_orthogonal;
	vaspect = p_vaspect;
	Vector<Plane> planes[2];

	/////////////////////////////////////////////////////////////////////////////
	// Figure out our center transform

	// 1. obtain our planes
	for (uint32_t v = 0; v < view_count; v++) {
		planes[v] = p_projections[v].get_projection_planes(p_transforms[v]);
	}

	// 2. average and normalize plane normals to obtain z vector, cross them to obtain y vector, and
	// from there the x vector for combined camera basis.
	Vector3 n0 = planes[0][Projection::PLANE_LEFT].normal;
	Vector3 n1 = planes[1][Projection::PLANE_RIGHT].normal;
	Vector3 z = (n0 + n1).normalized();
	Vector3 y = n0.cross(n1).normalized();
	Vector3 x = y.cross(z).normalized();
	y = z.cross(x).normalized();
	main_transform.basis.set_columns(x, y, z);

	// 3. create a horizon plane with one of the eyes and the up vector as normal.
	Plane horizon(y, p_transforms[0].origin);

	// 4. Intersect horizon, left and right to obtain the combined camera origin.
	ERR_FAIL_COND_MSG(!horizon.intersect_3(planes[0][Projection::PLANE_LEFT],
						  planes[1][Projection::PLANE_RIGHT], &main_transform.origin),
		"Can't determine camera origin");

	// handy to have the inverse of the transform we just build
	Transform3D main_transform_inv = main_transform.inverse();

	// 5. figure out far plane, this could use some improvement, we may have our far plane too close
	// like this, not sure if this matters
	Vector3 far_center = (planes[0][Projection::PLANE_FAR].get_center() +
							 planes[1][Projection::PLANE_FAR].get_center()) *
						 0.5;
	Plane far_plane = Plane(-z, far_center);

	/////////////////////////////////////////////////////////////////////////////
	// Figure out our top/bottom planes

	// 6. Intersect far and left planes with top planes from both eyes, save the point with highest
	// y as top_left.
	Vector3 top_left, other;
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[0][Projection::PLANE_LEFT],
						  planes[0][Projection::PLANE_TOP], &top_left),
		"Can't determine left camera far/left/top vector");
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[1][Projection::PLANE_LEFT],
						  planes[1][Projection::PLANE_TOP], &other),
		"Can't determine right camera far/left/top vector");
	if (y.dot(top_left) < y.dot(other)) {
		top_left = other;
	}

	// 7. Intersect far and left planes with bottom planes from both eyes, save the point with
	// lowest y as bottom_left.
	Vector3 bottom_left;
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[0][Projection::PLANE_LEFT],
						  planes[0][Projection::PLANE_BOTTOM], &bottom_left),
		"Can't determine left camera far/left/bottom vector");
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[1][Projection::PLANE_LEFT],
						  planes[1][Projection::PLANE_BOTTOM], &other),
		"Can't determine right camera far/left/bottom vector");
	if (y.dot(other) < y.dot(bottom_left)) {
		bottom_left = other;
	}

	// 8. Intersect far and right planes with top planes from both eyes, save the point with highest
	// y as top_right.
	Vector3 top_right;
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[0][Projection::PLANE_RIGHT],
						  planes[0][Projection::PLANE_TOP], &top_right),
		"Can't determine left camera far/right/top vector");
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[1][Projection::PLANE_RIGHT],
						  planes[1][Projection::PLANE_TOP], &other),
		"Can't determine right camera far/right/top vector");
	if (y.dot(top_right) < y.dot(other)) {
		top_right = other;
	}

	//  9. Intersect far and right planes with bottom planes from both eyes, save the point with
	//  lowest y as bottom_right.
	Vector3 bottom_right;
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[0][Projection::PLANE_RIGHT],
						  planes[0][Projection::PLANE_BOTTOM], &bottom_right),
		"Can't determine left camera far/right/bottom vector");
	ERR_FAIL_COND_MSG(!far_plane.intersect_3(planes[1][Projection::PLANE_RIGHT],
						  planes[1][Projection::PLANE_BOTTOM], &other),
		"Can't determine right camera far/right/bottom vector");
	if (y.dot(other) < y.dot(bottom_right)) {
		bottom_right = other;
	}

	// 10. Create top plane with these points: camera origin, top_left, top_right
	Plane top(main_transform.origin, top_left, top_right);

	// 11. Create bottom plane with these points: camera origin, bottom_left, bottom_right
	Plane bottom(main_transform.origin, bottom_left, bottom_right);

	/////////////////////////////////////////////////////////////////////////////
	// Figure out our near plane points

	// 12. Create a near plane using -camera z and the eye further along in that axis.
	Plane near_plane;
	Vector3 neg_z = -z;
	if (neg_z.dot(p_transforms[1].origin) < neg_z.dot(p_transforms[0].origin)) {
		near_plane = Plane(neg_z, p_transforms[0].origin);
	}
	else {
		near_plane = Plane(neg_z, p_transforms[1].origin);
	}

	// 13. Intersect near plane with bottm/left planes, to obtain min_vec then top/right to obtain
	// max_vec
	Vector3 min_vec;
	ERR_FAIL_COND_MSG(!near_plane.intersect_3(bottom, planes[0][Projection::PLANE_LEFT], &min_vec),
		"Can't determine left camera near/left/bottom vector");
	ERR_FAIL_COND_MSG(!near_plane.intersect_3(bottom, planes[1][Projection::PLANE_LEFT], &other),
		"Can't determine right camera near/left/bottom vector");
	if (x.dot(other) < x.dot(min_vec)) {
		min_vec = other;
	}

	Vector3 max_vec;
	ERR_FAIL_COND_MSG(!near_plane.intersect_3(top, planes[0][Projection::PLANE_RIGHT], &max_vec),
		"Can't determine left camera near/right/top vector");
	ERR_FAIL_COND_MSG(!near_plane.intersect_3(top, planes[1][Projection::PLANE_RIGHT], &other),
		"Can't determine right camera near/right/top vector");
	if (x.dot(max_vec) < x.dot(other)) {
		max_vec = other;
	}

	// 14. transform these points by the inverse camera to obtain local_min_vec and local_max_vec
	Vector3 local_min_vec = main_transform_inv.xform(min_vec);
	Vector3 local_max_vec = main_transform_inv.xform(max_vec);

	// 15. get x and y from these to obtain left, top, right bottom for the frustum. Get the
	// distance from near plane to camera origin to obtain near, and the distance from the far plane
	// to the camera origin to obtain far.
	float z_near = -near_plane.distance_to(main_transform.origin);
	float z_far = -far_plane.distance_to(main_transform.origin);

	// 16. Use this to build the combined camera matrix.
	main_projection.set_frustum(
		local_min_vec.x, local_max_vec.x, local_min_vec.y, local_max_vec.y, z_near, z_far);

	// 3. Copy our view data
	for (uint32_t v = 0; v < view_count; v++) {
		view_offset[v] = main_transform_inv * p_transforms[v];
		view_projection[v] = p_projections[v] * Projection(view_offset[v].inverse());
	}
}

RID RendererSceneRender::compositor_effect_allocate()
{
	return RendererCompositorStorage::compositor_effect_allocate();
}

void RendererSceneRender::compositor_effect_initialize(RID p_rid)
{
	RendererCompositorStorage::compositor_effect_initialize(p_rid);
}

void RendererSceneRender::compositor_effect_free(RID p_rid)
{
	RendererCompositorStorage::compositor_effect_free(p_rid);
}

bool RendererSceneRender::is_compositor_effect(RID p_effect)
{
	return RendererCompositorStorage::is_compositor_effect(p_effect);
}

void RendererSceneRender::compositor_effect_set_enabled(RID p_effect, bool p_enabled)
{
	RendererCompositorStorage::compositor_effect_set_enabled(p_effect, p_enabled);
}

void RendererSceneRender::compositor_effect_set_flag(
	RID p_effect, RSE::CompositorEffectFlags p_flag, bool p_set)
{
	RendererCompositorStorage::compositor_effect_set_flag(p_effect, p_flag, p_set);
}

RID RendererSceneRender::compositor_allocate() { return RendererCompositorStorage::compositor_allocate(); }

void RendererSceneRender::compositor_initialize(RID p_rid)
{
	RendererCompositorStorage::compositor_initialize(p_rid);
}

void RendererSceneRender::compositor_free(RID p_rid) { RendererCompositorStorage::compositor_free(p_rid); }

bool RendererSceneRender::is_compositor(RID p_rid)
{
	return RendererCompositorStorage::is_compositor(p_rid);
}

RID RendererSceneRender::environment_allocate()
{
	return RendererEnvironmentStorage::environment_allocate();
}

void RendererSceneRender::environment_initialize(RID p_rid)
{
	RendererEnvironmentStorage::environment_initialize(p_rid);
}

void RendererSceneRender::environment_free(RID p_rid)
{
	RendererEnvironmentStorage::environment_free(p_rid);
}

bool RendererSceneRender::is_environment(RID p_rid)
{
	return RendererEnvironmentStorage::is_environment(p_rid);
}

void RendererSceneRender::environment_set_background(RID p_env, RSE::EnvironmentBG p_bg)
{
	RendererEnvironmentStorage::environment_set_background(p_env, p_bg);
}

void RendererSceneRender::environment_set_sky(RID p_env, RID p_sky)
{
	RendererEnvironmentStorage::environment_set_sky(p_env, p_sky);
}

void RendererSceneRender::environment_set_sky_custom_fov(RID p_env, float p_scale)
{
	RendererEnvironmentStorage::environment_set_sky_custom_fov(p_env, p_scale);
}

void RendererSceneRender::environment_set_sky_orientation(RID p_env, const Basis& p_orientation)
{
	RendererEnvironmentStorage::environment_set_sky_orientation(p_env, p_orientation);
}

void RendererSceneRender::environment_set_bg_color(RID p_env, const Color& p_color)
{
	RendererEnvironmentStorage::environment_set_bg_color(p_env, p_color);
}

void RendererSceneRender::environment_set_bg_energy(
	RID p_env, float p_multiplier, float p_exposure_value)
{
	RendererEnvironmentStorage::environment_set_bg_energy(p_env, p_multiplier, p_exposure_value);
}

void RendererSceneRender::environment_set_canvas_max_layer(RID p_env, int p_max_layer)
{
	RendererEnvironmentStorage::environment_set_canvas_max_layer(p_env, p_max_layer);
}

void RendererSceneRender::environment_set_ambient_light(RID p_env, const Color& p_color,
	RSE::EnvironmentAmbientSource p_ambient, float p_energy, float p_sky_contribution,
	RSE::EnvironmentReflectionSource p_reflection_source)
{
	RendererEnvironmentStorage::environment_set_ambient_light(
		p_env, p_color, p_ambient, p_energy, p_sky_contribution, p_reflection_source);
}

RSE::EnvironmentBG RendererSceneRender::environment_get_background(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_background(p_env);
}

RID RendererSceneRender::environment_get_sky(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sky(p_env);
}

float RendererSceneRender::environment_get_sky_custom_fov(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sky_custom_fov(p_env);
}

Basis RendererSceneRender::environment_get_sky_orientation(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sky_orientation(p_env);
}

Color RendererSceneRender::environment_get_bg_color(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_bg_color(p_env);
}

float RendererSceneRender::environment_get_bg_energy_multiplier(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_bg_energy_multiplier(p_env);
}

float RendererSceneRender::environment_get_bg_intensity(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_bg_intensity(p_env);
}

int RendererSceneRender::environment_get_canvas_max_layer(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_canvas_max_layer(p_env);
}

RSE::EnvironmentAmbientSource RendererSceneRender::environment_get_ambient_source(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ambient_source(p_env);
}

Color RendererSceneRender::environment_get_ambient_light(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ambient_light(p_env);
}

float RendererSceneRender::environment_get_ambient_light_energy(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ambient_light_energy(p_env);
}

float RendererSceneRender::environment_get_ambient_sky_contribution(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ambient_sky_contribution(p_env);
}

RSE::EnvironmentReflectionSource RendererSceneRender::environment_get_reflection_source(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_reflection_source(p_env);
}

void RendererSceneRender::environment_set_camera_feed_id(RID p_env, int p_camera_feed_id)
{
	RendererEnvironmentStorage::environment_set_camera_feed_id(p_env, p_camera_feed_id);
}

int RendererSceneRender::environment_get_camera_feed_id(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_camera_feed_id(p_env);
}

void RendererSceneRender::environment_set_tonemap(
	RID p_env, RSE::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white)
{
	RendererEnvironmentStorage::environment_set_tonemap(p_env, p_tone_mapper, p_exposure, p_white);
}

RSE::EnvironmentToneMapper RendererSceneRender::environment_get_tone_mapper(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_tone_mapper(p_env);
}

float RendererSceneRender::environment_get_exposure(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_exposure(p_env);
}

float RendererSceneRender::environment_get_white(
	RID p_env, bool p_limit_agx_white, float p_output_max_value)
{
	return RendererEnvironmentStorage::environment_get_white(p_env, p_limit_agx_white, p_output_max_value);
}

void RendererSceneRender::environment_set_tonemap_agx_contrast(RID p_env, float p_agx_contrast)
{
	RendererEnvironmentStorage::environment_set_tonemap_agx_contrast(p_env, p_agx_contrast);
}

float RendererSceneRender::environment_get_tonemap_agx_contrast(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_tonemap_agx_contrast(p_env);
}

RendererEnvironmentStorage::TonemapParameters
RendererSceneRender::environment_get_tonemap_parameters(
	RID p_env, bool p_limit_agx_white, float p_output_max_value)
{
	return RendererEnvironmentStorage::environment_get_tonemap_parameters(
		p_env, p_limit_agx_white, p_output_max_value);
}

void RendererSceneRender::environment_set_fog(RID p_env, bool p_enable, const Color& p_light_color,
	float p_light_energy, float p_sun_scatter, float p_density, float p_height,
	float p_height_density, float p_aerial_perspective, float p_sky_affect,
	RSE::EnvironmentFogMode p_mode)
{
	RendererEnvironmentStorage::environment_set_fog(p_env, p_enable, p_light_color, p_light_energy,
		p_sun_scatter, p_density, p_height, p_height_density, p_aerial_perspective, p_sky_affect,
		p_mode);
}

bool RendererSceneRender::environment_get_fog_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_enabled(p_env);
}

RSE::EnvironmentFogMode RendererSceneRender::environment_get_fog_mode(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_mode(p_env);
}

Color RendererSceneRender::environment_get_fog_light_color(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_light_color(p_env);
}

float RendererSceneRender::environment_get_fog_light_energy(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_light_energy(p_env);
}

float RendererSceneRender::environment_get_fog_sun_scatter(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_sun_scatter(p_env);
}

float RendererSceneRender::environment_get_fog_density(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_density(p_env);
}

float RendererSceneRender::environment_get_fog_sky_affect(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_sky_affect(p_env);
}

float RendererSceneRender::environment_get_fog_height(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_height(p_env);
}

float RendererSceneRender::environment_get_fog_height_density(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_height_density(p_env);
}

float RendererSceneRender::environment_get_fog_aerial_perspective(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_aerial_perspective(p_env);
}

void RendererSceneRender::environment_set_fog_depth(
	RID p_env, float p_curve, float p_begin, float p_end)
{
	RendererEnvironmentStorage::environment_set_fog_depth(p_env, p_curve, p_begin, p_end);
}

float RendererSceneRender::environment_get_fog_depth_curve(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_depth_curve(p_env);
}

float RendererSceneRender::environment_get_fog_depth_begin(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_depth_begin(p_env);
}

float RendererSceneRender::environment_get_fog_depth_end(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_fog_depth_end(p_env);
}

void RendererSceneRender::environment_set_volumetric_fog(RID p_env, bool p_enable, float p_density,
	const Color& p_albedo, const Color& p_emission, float p_emission_energy, float p_anisotropy,
	float p_length, float p_detail_spread, float p_gi_inject, bool p_temporal_reprojection,
	float p_temporal_reprojection_amount, float p_ambient_inject, float p_sky_affect)
{
	RendererEnvironmentStorage::environment_set_volumetric_fog(p_env, p_enable, p_density, p_albedo,
		p_emission, p_emission_energy, p_anisotropy, p_length, p_detail_spread, p_gi_inject,
		p_temporal_reprojection, p_temporal_reprojection_amount, p_ambient_inject, p_sky_affect);
}

bool RendererSceneRender::environment_get_volumetric_fog_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_enabled(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_density(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_density(p_env);
}

Color RendererSceneRender::environment_get_volumetric_fog_scattering(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_scattering(p_env);
}

Color RendererSceneRender::environment_get_volumetric_fog_emission(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_emission(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_emission_energy(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_emission_energy(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_anisotropy(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_anisotropy(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_length(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_length(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_detail_spread(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_detail_spread(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_gi_inject(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_gi_inject(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_sky_affect(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_sky_affect(p_env);
}

bool RendererSceneRender::environment_get_volumetric_fog_temporal_reprojection(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_temporal_reprojection(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_temporal_reprojection_amount(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_temporal_reprojection_amount(p_env);
}

float RendererSceneRender::environment_get_volumetric_fog_ambient_inject(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_volumetric_fog_ambient_inject(p_env);
}

void RendererSceneRender::environment_set_glow(RID p_env, bool p_enable, Vector<float> p_levels,
	float p_intensity, float p_strength, float p_mix, float p_bloom_threshold,
	RSE::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold,
	float p_hdr_bleed_scale, float p_hdr_luminance_cap, float p_glow_map_strength, RID p_glow_map)
{
	RendererEnvironmentStorage::environment_set_glow(p_env, p_enable, p_levels, p_intensity, p_strength,
		p_mix, p_bloom_threshold, p_blend_mode, p_hdr_bleed_threshold, p_hdr_bleed_scale,
		p_hdr_luminance_cap, p_glow_map_strength, p_glow_map);
}

bool RendererSceneRender::environment_get_glow_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_enabled(p_env);
}

Vector<float> RendererSceneRender::environment_get_glow_levels(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_levels(p_env);
}

float RendererSceneRender::environment_get_glow_intensity(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_intensity(p_env);
}

float RendererSceneRender::environment_get_glow_strength(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_strength(p_env);
}

float RendererSceneRender::environment_get_glow_bloom(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_bloom(p_env);
}

float RendererSceneRender::environment_get_glow_mix(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_mix(p_env);
}

RSE::EnvironmentGlowBlendMode RendererSceneRender::environment_get_glow_blend_mode(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_blend_mode(p_env);
}

float RendererSceneRender::environment_get_glow_hdr_bleed_threshold(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_hdr_bleed_threshold(p_env);
}

float RendererSceneRender::environment_get_glow_hdr_luminance_cap(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_hdr_luminance_cap(p_env);
}

float RendererSceneRender::environment_get_glow_hdr_bleed_scale(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_hdr_bleed_scale(p_env);
}

float RendererSceneRender::environment_get_glow_map_strength(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_map_strength(p_env);
}

RID RendererSceneRender::environment_get_glow_map(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_glow_map(p_env);
}

void RendererSceneRender::environment_set_ssr(RID p_env, bool p_enable, int p_max_steps,
	float p_fade_int, float p_fade_out, float p_depth_tolerance)
{
	RendererEnvironmentStorage::environment_set_ssr(
		p_env, p_enable, p_max_steps, p_fade_int, p_fade_out, p_depth_tolerance);
}

bool RendererSceneRender::environment_get_ssr_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssr_enabled(p_env);
}

int RendererSceneRender::environment_get_ssr_max_steps(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssr_max_steps(p_env);
}

float RendererSceneRender::environment_get_ssr_fade_in(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssr_fade_in(p_env);
}

float RendererSceneRender::environment_get_ssr_fade_out(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssr_fade_out(p_env);
}

float RendererSceneRender::environment_get_ssr_depth_tolerance(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssr_depth_tolerance(p_env);
}

void RendererSceneRender::environment_set_ssao(RID p_env, bool p_enable, float p_radius,
	float p_intensity, float p_power, float p_detail, float p_horizon, float p_sharpness,
	float p_light_affect, float p_ao_channel_affect)
{
	RendererEnvironmentStorage::environment_set_ssao(p_env, p_enable, p_radius, p_intensity, p_power,
		p_detail, p_horizon, p_sharpness, p_light_affect, p_ao_channel_affect);
}

bool RendererSceneRender::environment_get_ssao_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_enabled(p_env);
}

float RendererSceneRender::environment_get_ssao_radius(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_radius(p_env);
}

float RendererSceneRender::environment_get_ssao_intensity(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_intensity(p_env);
}

float RendererSceneRender::environment_get_ssao_power(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_power(p_env);
}

float RendererSceneRender::environment_get_ssao_detail(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_detail(p_env);
}

float RendererSceneRender::environment_get_ssao_horizon(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_horizon(p_env);
}

float RendererSceneRender::environment_get_ssao_sharpness(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_sharpness(p_env);
}

float RendererSceneRender::environment_get_ssao_direct_light_affect(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_direct_light_affect(p_env);
}

float RendererSceneRender::environment_get_ssao_ao_channel_affect(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssao_ao_channel_affect(p_env);
}

void RendererSceneRender::environment_set_ssil(RID p_env, bool p_enable, float p_radius,
	float p_intensity, float p_sharpness, float p_normal_rejection)
{
	RendererEnvironmentStorage::environment_set_ssil(
		p_env, p_enable, p_radius, p_intensity, p_sharpness, p_normal_rejection);
}

bool RendererSceneRender::environment_get_ssil_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssil_enabled(p_env);
}

float RendererSceneRender::environment_get_ssil_radius(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssil_radius(p_env);
}

float RendererSceneRender::environment_get_ssil_intensity(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssil_intensity(p_env);
}

float RendererSceneRender::environment_get_ssil_sharpness(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssil_sharpness(p_env);
}

float RendererSceneRender::environment_get_ssil_normal_rejection(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_ssil_normal_rejection(p_env);
}

void RendererSceneRender::environment_set_sdfgi(RID p_env, bool p_enable, int p_cascades,
	float p_min_cell_size, RSE::EnvironmentSDFGIYScale p_y_scale, bool p_use_occlusion,
	float p_bounce_feedback, bool p_read_sky, float p_energy, float p_normal_bias,
	float p_probe_bias)
{
	RendererEnvironmentStorage::environment_set_sdfgi(p_env, p_enable, p_cascades, p_min_cell_size,
		p_y_scale, p_use_occlusion, p_bounce_feedback, p_read_sky, p_energy, p_normal_bias,
		p_probe_bias);
}

bool RendererSceneRender::environment_get_sdfgi_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_enabled(p_env);
}

int RendererSceneRender::environment_get_sdfgi_cascades(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_cascades(p_env);
}

float RendererSceneRender::environment_get_sdfgi_min_cell_size(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_min_cell_size(p_env);
}

bool RendererSceneRender::environment_get_sdfgi_use_occlusion(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_use_occlusion(p_env);
}

float RendererSceneRender::environment_get_sdfgi_bounce_feedback(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_bounce_feedback(p_env);
}

bool RendererSceneRender::environment_get_sdfgi_read_sky_light(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_read_sky_light(p_env);
}

float RendererSceneRender::environment_get_sdfgi_energy(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_energy(p_env);
}

float RendererSceneRender::environment_get_sdfgi_normal_bias(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_normal_bias(p_env);
}

float RendererSceneRender::environment_get_sdfgi_probe_bias(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_probe_bias(p_env);
}

RSE::EnvironmentSDFGIYScale RendererSceneRender::environment_get_sdfgi_y_scale(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_sdfgi_y_scale(p_env);
}

void RendererSceneRender::environment_set_adjustment(RID p_env, bool p_enable, float p_brightness,
	float p_contrast, float p_saturation, bool p_use_1d_color_correction, RID p_color_correction)
{
	RendererEnvironmentStorage::environment_set_adjustment(p_env, p_enable, p_brightness, p_contrast,
		p_saturation, p_use_1d_color_correction, p_color_correction);
}

bool RendererSceneRender::environment_get_adjustments_enabled(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_adjustments_enabled(p_env);
}

float RendererSceneRender::environment_get_adjustments_brightness(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_adjustments_brightness(p_env);
}

float RendererSceneRender::environment_get_adjustments_contrast(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_adjustments_contrast(p_env);
}

float RendererSceneRender::environment_get_adjustments_saturation(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_adjustments_saturation(p_env);
}

bool RendererSceneRender::environment_get_use_1d_color_correction(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_use_1d_color_correction(p_env);
}

RID RendererSceneRender::environment_get_color_correction(RID p_env)
{
	return RendererEnvironmentStorage::environment_get_color_correction(p_env);
}

RenderGeometryInstance* RendererSceneRender::geometry_instance_create(RID p_base)
{
	return nullptr;
}

void RendererSceneRender::geometry_instance_free(RenderGeometryInstance* p_geometry_instance) {}

uint32_t RendererSceneRender::geometry_instance_get_pair_mask() { return 0; }

uint32_t RendererSceneRender::get_max_lights_total() { return 0; }

uint32_t RendererSceneRender::get_max_lights_per_mesh() { return 0; }

void RendererSceneRender::mesh_generate_pipelines(RID p_mesh, bool p_background_compilation) {}

uint32_t RendererSceneRender::get_pipeline_compilations(
	RenderingServerEnums::PipelineSource p_source)
{
	return 0;
}

void RendererSceneRender::sdfgi_update(const Ref<RenderSceneBuffers>& p_render_buffers,
	RID p_environment, const Vector3& p_world_position)
{
}

int RendererSceneRender::sdfgi_get_pending_region_count(
	const Ref<RenderSceneBuffers>& p_render_buffers) const
{
	return 0;
}

AABB RendererSceneRender::sdfgi_get_pending_region_bounds(
	const Ref<RenderSceneBuffers>& p_render_buffers, int p_region) const
{
	return AABB();
}

uint32_t RendererSceneRender::sdfgi_get_pending_region_cascade(
	const Ref<RenderSceneBuffers>& p_render_buffers, int p_region) const
{
	return 0;
}

void RendererSceneRender::environment_set_ssr_half_size(bool p_enable) {}

void RendererSceneRender::environment_set_ssr_roughness_quality(
	RenderingServerEnums::EnvironmentSSRRoughnessQuality p_quality)
{
}

void RendererSceneRender::environment_set_ssao_quality(
	RenderingServerEnums::EnvironmentSSAOQuality p_quality, bool p_half_size,
	float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to)
{
}

void RendererSceneRender::environment_set_ssil_quality(
	RenderingServerEnums::EnvironmentSSILQuality p_quality, bool p_half_size,
	float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to)
{
}

void RendererSceneRender::sub_surface_scattering_set_quality(
	RenderingServerEnums::SubSurfaceScatteringQuality p_quality)
{
}

void RendererSceneRender::sub_surface_scattering_set_scale(float p_scale, float p_depth_scale) {}

RID RendererSceneRender::sky_allocate() { return RID(); }

void RendererSceneRender::sky_initialize(RID p_rid) {}

void RendererSceneRender::sky_set_radiance_size(RID p_sky, int p_radiance_size) {}

void RendererSceneRender::sky_set_mode(RID p_sky, RenderingServerEnums::SkyMode p_mode) {}

void RendererSceneRender::sky_set_material(RID p_sky, RID p_material) {}

Ref<Image> RendererSceneRender::sky_bake_panorama(
	RID p_sky, float p_energy, bool p_bake_irradiance, const Vector2i& p_size)
{
	return Ref<Image>();
}

void RendererSceneRender::environment_set_volumetric_fog_volume_size(int p_size, int p_depth) {}

void RendererSceneRender::environment_set_volumetric_fog_filter_active(bool p_enable) {}

void RendererSceneRender::environment_glow_set_use_bicubic_upscale(bool p_enable) {}

void RendererSceneRender::environment_set_sdfgi_ray_count(
	RenderingServerEnums::EnvironmentSDFGIRayCount p_count)
{
}

void RendererSceneRender::environment_set_sdfgi_frames_to_converge(
	RenderingServerEnums::EnvironmentSDFGIFramesToConverge p_frames)
{
}

void RendererSceneRender::environment_set_sdfgi_frames_to_update_light(
	RenderingServerEnums::EnvironmentSDFGIFramesToUpdateLight p_frames)
{
}

Ref<Image> RendererSceneRender::environment_bake_panorama(
	RID p_env, bool p_bake_irradiance, const Vector2i& p_size)
{
	return Ref<Image>();
}

void RendererSceneRender::positional_soft_shadow_filter_set_quality(
	RenderingServerEnums::ShadowQuality p_quality)
{
}

void RendererSceneRender::directional_soft_shadow_filter_set_quality(
	RenderingServerEnums::ShadowQuality p_quality)
{
}

RID RendererSceneRender::fog_volume_instance_create(RID p_fog_volume) { return RID(); }

void RendererSceneRender::fog_volume_instance_set_transform(
	RID p_instance, const Transform3D& p_transform)
{
}

void RendererSceneRender::fog_volume_instance_set_active(RID p_instance, bool p_active) {}

RID RendererSceneRender::fog_volume_instance_get_volume(RID p_instance) const { return RID(); }

Vector3 RendererSceneRender::fog_volume_instance_get_position(RID p_instance) const
{
	return Vector3();
}

RID RendererSceneRender::voxel_gi_instance_create(RID p_base) { return RID(); }

void RendererSceneRender::voxel_gi_instance_set_transform_to_data(
	RID p_probe, const Transform3D& p_xform)
{
}

bool RendererSceneRender::voxel_gi_needs_update(RID p_probe) const { return false; }

void RendererSceneRender::voxel_gi_update(RID p_probe, bool p_update_light_instances,
	const Vector<RID>& p_light_instances,
	const PagedArray<RenderGeometryInstance*>& p_dynamic_objects)
{
}

void RendererSceneRender::voxel_gi_set_quality(RenderingServerEnums::VoxelGIQuality p_quality) {}

void RendererSceneRender::render_scene(const Ref<RenderSceneBuffers>& p_render_buffers,
	const CameraData* p_camera_data, const CameraData* p_prev_camera_data,
	const PagedArray<RenderGeometryInstance*>& p_instances, const PagedArray<RID>& p_lights,
	const PagedArray<RID>& p_reflection_probes, const PagedArray<RID>& p_voxel_gi_instances,
	const PagedArray<RID>& p_decals, const PagedArray<RID>& p_lightmaps,
	const PagedArray<RID>& p_fog_volumes, RID p_environment, RID p_camera_attributes,
	RID p_compositor, RID p_shadow_atlas, RID p_occlusion_buffer, RID p_reflection_atlas,
	RID p_reflection_probe, int p_reflection_probe_pass, float p_screen_mesh_lod_threshold,
	const RenderShadowData* p_render_shadow_data, int p_render_shadow_count,
	const RenderSDFGIData* p_render_sdfgi_data, int p_render_sdfgi_count,
	float p_sdfgi_cascade_size, const RenderSDFGIUpdateData* p_render_sdfgi_update_data,
	RenderingServerTypes::RenderInfo* p_render_info)
{
}

void RendererSceneRender::render_material(const Transform3D& p_cam_transform,
	const Projection& p_cam_projection, bool p_cam_orthogonal,
	const PagedArray<RenderGeometryInstance*>& p_instances, RID p_framebuffer,
	const Rect2i& p_region)
{
}

void RendererSceneRender::render_particle_collider_heightfield(RID p_collider,
	const Transform3D& p_transform, const PagedArray<RenderGeometryInstance*>& p_instances)
{
}

void RendererSceneRender::set_scene_pass(uint64_t p_pass) {}

void RendererSceneRender::set_time(double p_time, double p_step) {}

void RendererSceneRender::set_debug_draw_mode(RenderingServerEnums::ViewportDebugDraw p_debug_draw)
{
}

Ref<RenderSceneBuffers> RendererSceneRender::render_buffers_create()
{
	return Ref<RenderSceneBuffers>();
}

void RendererSceneRender::gi_set_use_half_resolution(bool p_enable) {}

void RendererSceneRender::screen_space_roughness_limiter_set_active(
	bool p_enable, float p_amount, float p_limit)
{
}

bool RendererSceneRender::screen_space_roughness_limiter_is_active() { return false; }

bool RendererSceneRender::free(RID p_rid) { return false; }

void RendererSceneRender::sdfgi_set_debug_probe_select(
	const Vector3& p_position, const Vector3& p_dir)
{
}

void RendererSceneRender::decals_set_filter(RenderingServerEnums::DecalFilter p_filter) {}

void RendererSceneRender::light_projectors_set_filter(
	RenderingServerEnums::LightProjectorFilter p_filter)
{
}

void RendererSceneRender::lightmaps_set_bicubic_filter(bool p_enable) {}

void RendererSceneRender::material_set_use_debanding(bool p_enable) {}

void RendererSceneRender::update() {}

// --- Merged from servers/rendering/renderer_compositor.cpp ---

Error RendererCompositor::create()
{
	String driver_name = "";
	if (DisplayServer::is_initialized()) {
		driver_name = OS::get_current_rendering_driver_name();
	}

	if (driver_name == "dummy" || driver_name.is_empty()) {
		RasterizerDummy::make_current();
		return OK;
	}

#if defined(GLES3_ENABLED)
	if (driver_name == "opengl3" || driver_name == "opengl3_es" || driver_name == "opengl3_angle") {
		RasterizerGLES3::make_current(driver_name == "opengl3");
		return OK;
	}
#endif

#if defined(RD_ENABLED)
	if (!low_end) {
		low_end = false;
		bind_compositor<RendererCompositorRD>();
		if (DisplayServer::is_initialized()) {
			Error err = DisplayServer::init_rendering_device();
			ERR_FAIL_COND_V_MSG(
				err != OK, err, "Failed to initialize RenderingDevice via DisplayServer.");
		}
		return OK;
	}
#endif

	RasterizerDummy::make_current();
	return OK;
}

bool RendererCompositor::is_xr_enabled()
{
#ifndef XR_DISABLED
	return xr_enabled;
#else
	return false;
#endif
}

void RendererCompositorRD::bind_utilities()
{
	RendererUtilities::visibility_notifier_allocate =
		&RendererRD::Utilities::visibility_notifier_allocate;
	RendererUtilities::visibility_notifier_initialize =
		&RendererRD::Utilities::visibility_notifier_initialize;
	RendererUtilities::visibility_notifier_free = &RendererRD::Utilities::visibility_notifier_free;
	RendererUtilities::visibility_notifier_set_aabb =
		&RendererRD::Utilities::visibility_notifier_set_aabb;
	RendererUtilities::visibility_notifier_get_aabb =
		&RendererRD::Utilities::visibility_notifier_get_aabb;
	RendererUtilities::visibility_notifier_call = &RendererRD::Utilities::visibility_notifier_call;

	RendererUtilities::free = &RendererRD::Utilities::free;
	RendererUtilities::get_base_type = &RendererRD::Utilities::get_base_type;
	RendererUtilities::base_update_dependency = &RendererRD::Utilities::base_update_dependency;

	RendererUtilities::capture_timestamps_begin = &RendererRD::Utilities::capture_timestamps_begin;
	RendererUtilities::capture_timestamp = &RendererRD::Utilities::capture_timestamp;
	RendererUtilities::get_captured_timestamps_count =
		&RendererRD::Utilities::get_captured_timestamps_count;
	RendererUtilities::get_captured_timestamps_frame =
		&RendererRD::Utilities::get_captured_timestamps_frame;
	RendererUtilities::get_captured_timestamp_gpu_time =
		&RendererRD::Utilities::get_captured_timestamp_gpu_time;
	RendererUtilities::get_captured_timestamp_cpu_time =
		&RendererRD::Utilities::get_captured_timestamp_cpu_time;
	RendererUtilities::get_captured_timestamp_name =
		&RendererRD::Utilities::get_captured_timestamp_name;

	RendererUtilities::update_dirty_resources = &RendererRD::Utilities::update_dirty_resources;
	RendererUtilities::set_debug_generate_wireframes =
		&RendererRD::Utilities::set_debug_generate_wireframes;
	RendererUtilities::has_os_feature = &RendererRD::Utilities::has_os_feature;
	RendererUtilities::update_memory_info = &RendererRD::Utilities::update_memory_info;

	RendererUtilities::get_rendering_info = &RendererRD::Utilities::get_rendering_info;
	RendererUtilities::get_video_adapter_name = &RendererRD::Utilities::get_video_adapter_name;
	RendererUtilities::get_video_adapter_vendor = &RendererRD::Utilities::get_video_adapter_vendor;
	RendererUtilities::get_video_adapter_type = &RendererRD::Utilities::get_video_adapter_type;
	RendererUtilities::get_video_adapter_api_version =
		&RendererRD::Utilities::get_video_adapter_api_version;

	RendererUtilities::get_maximum_viewport_size =
		&RendererRD::Utilities::get_maximum_viewport_size;
	RendererUtilities::get_maximum_shader_varyings =
		&RendererRD::Utilities::get_maximum_shader_varyings;
	RendererUtilities::get_maximum_uniform_buffer_size =
		&RendererRD::Utilities::get_maximum_uniform_buffer_size;
}

// --- Merged from servers/rendering/renderer_canvas_render.cpp ---

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
			const Item::CommandRect* crect = reinterpret_cast<const Item::CommandRect*>(c);
			r = crect->rect;

		} break;
		case Item::Command::TYPE_NINEPATCH: {
			const Item::CommandNinePatch* style = reinterpret_cast<const Item::CommandNinePatch*>(c);
			r = style->rect;
		} break;

		case Item::Command::TYPE_POLYGON: {
			const Item::CommandPolygon* polygon = reinterpret_cast<const Item::CommandPolygon*>(c);
			r = polygon->polygon.rect_cache;
		} break;
		case Item::Command::TYPE_PRIMITIVE: {
			const Item::CommandPrimitive* primitive = reinterpret_cast<const Item::CommandPrimitive*>(c);
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
			const Item::CommandMesh* mesh = reinterpret_cast<const Item::CommandMesh*>(c);
			AABB aabb = RendererMeshStorage::mesh_get_aabb(mesh->mesh, skeleton);

			r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);

		} break;
		case Item::Command::TYPE_MULTIMESH: {
			const Item::CommandMultiMesh* multimesh = reinterpret_cast<const Item::CommandMultiMesh*>(c);
			AABB aabb = RendererMeshStorage::multimesh_get_aabb(multimesh->multimesh);

			r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);

		} break;
		case Item::Command::TYPE_PARTICLES: {
			const Item::CommandParticles* particles_cmd =
				reinterpret_cast<const Item::CommandParticles*>(c);
			if (particles_cmd->particles.is_valid()) {
				AABB aabb = Renderer::particles_storage->particles_get_aabb(particles_cmd->particles);
				r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);
			}

		} break;
		case Item::Command::TYPE_TRANSFORM: {
			const Item::CommandTransform* transform = reinterpret_cast<const Item::CommandTransform*>(c);
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

// --- Merged from servers/rendering/renderer_geometry_instance.cpp ---

void RenderGeometryInstance::set_skeleton(RID p_skeleton) {
	if (!data) {
		return;
	}
	data->skeleton = p_skeleton;

	_mark_dirty();
	data->dirty_dependencies = true;
}

void RenderGeometryInstance::set_material_override(RID p_override) {
	if (!data) {
		return;
	}
	data->material_override = p_override;

	_mark_dirty();
	data->dirty_dependencies = true;
}

void RenderGeometryInstance::set_material_overlay(RID p_overlay) {
	if (!data) {
		return;
	}
	data->material_overlay = p_overlay;

	_mark_dirty();
	data->dirty_dependencies = true;
}

void RenderGeometryInstance::set_surface_materials(const Vector<RID> &p_materials) {
	if (!data) {
		return;
	}
	data->surface_materials = p_materials;

	_mark_dirty();
	data->dirty_dependencies = true;
}

void RenderGeometryInstance::set_mesh_instance(RID p_mesh_instance) {
	mesh_instance = p_mesh_instance;

	_mark_dirty();
}

void RenderGeometryInstance::set_transform(const Transform3D &p_transform, const AABB &p_aabb, const AABB &p_transformed_aabb) {
	transform = p_transform;
	mirror = p_transform.basis.determinant() < 0;
	if (data) {
		data->aabb = p_aabb;
	}
	transformed_aabb = p_transformed_aabb;

	Vector3 model_scale_vec = p_transform.basis.get_scale_abs();
	// handle non uniform scale here

	float max_scale = MAX(model_scale_vec.x, MAX(model_scale_vec.y, model_scale_vec.z));
	float min_scale = MIN(model_scale_vec.x, MIN(model_scale_vec.y, model_scale_vec.z));
	non_uniform_scale = max_scale >= 0.0 && (min_scale / max_scale) < 0.999;

	lod_model_scale = max_scale;
}

void RenderGeometryInstance::set_pivot_data(float p_sorting_offset, bool p_use_aabb_center) {
	sorting_offset = p_sorting_offset;
	use_aabb_center = p_use_aabb_center;
}

void RenderGeometryInstance::set_lod_bias(float p_lod_bias) {
	lod_bias = p_lod_bias;
}

void RenderGeometryInstance::set_layer_mask(uint32_t p_layer_mask) {
	layer_mask = p_layer_mask;
}

void RenderGeometryInstance::set_fade_range(bool p_enable_near, float p_near_begin, float p_near_end, bool p_enable_far, float p_far_begin, float p_far_end) {
	fade_near = p_enable_near;
	fade_near_begin = p_near_begin;
	fade_near_end = p_near_end;
	fade_far = p_enable_far;
	fade_far_begin = p_far_begin;
	fade_far_end = p_far_end;
}

void RenderGeometryInstance::set_parent_fade_alpha(float p_alpha) {
	parent_fade_alpha = p_alpha;
}

void RenderGeometryInstance::set_transparency(float p_transparency) {
	force_alpha = CLAMP(1.0 - p_transparency, 0, 1);
}

void RenderGeometryInstance::set_use_baked_light(bool p_enable) {
	if (!data) {
		return;
	}
	data->use_baked_light = p_enable;

	_mark_dirty();
}

void RenderGeometryInstance::set_use_dynamic_gi(bool p_enable) {
	if (!data) {
		return;
	}
	data->use_dynamic_gi = p_enable;

	_mark_dirty();
}

void RenderGeometryInstance::set_instance_shader_uniforms_offset(int32_t p_offset) {
	shader_uniforms_offset = p_offset;

	_mark_dirty();
}

void RenderGeometryInstance::set_cast_double_sided_shadows(bool p_enable) {
	if (!data) {
		return;
	}
	data->cast_double_sided_shadows = p_enable;

	_mark_dirty();
}

void RenderGeometryInstance::reset_motion_vectors() {
}

Transform3D RenderGeometryInstance::get_transform() {
	return transform;
}

AABB RenderGeometryInstance::get_aabb() {
	return data ? data->aabb : AABB();
}
