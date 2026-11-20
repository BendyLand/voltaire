/**************************************************************************/
/*  lightmapper.cpp                                                       */
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

#include "lightmapper.h"

#include "modules/modules_enabled.gen.h"

Ref<LightmapDenoiser> (*LightmapDenoiser::create_function)() = nullptr;

Ref<LightmapDenoiser> LightmapDenoiser::create()
{
	if (create_function) {
		return Ref<LightmapDenoiser>(create_function());
	}
	return Ref<LightmapDenoiser>();
}

#if !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)

Ref<LightmapRaycaster> LightmapRaycaster::create() { return Ref<LightmapRaycaster>(); }

bool LightmapRaycaster::intersect(Ray& p_ray) { return false; }

void LightmapRaycaster::intersect(Vector<Ray>& r_rays) {}

void LightmapRaycaster::add_mesh(const Vector<Vector3>& p_vertices,
	const Vector<Vector3>& p_normals, const Vector<Vector2>& p_uv2s, unsigned int p_id)
{
}

void LightmapRaycaster::set_mesh_alpha_texture(Ref<Image> p_alpha_texture, unsigned int p_id) {}

void LightmapRaycaster::commit() {}

void LightmapRaycaster::set_mesh_filter(const HashSet<int>& p_mesh_ids) {}

void LightmapRaycaster::clear_mesh_filter() {}

LightmapRaycaster::LightmapRaycaster() {}

LightmapRaycaster::~LightmapRaycaster() {}

#endif // !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)

#ifndef MODULE_LIGHTMAPPER_RD_ENABLED

Ref<Lightmapper> Lightmapper::create() { return Ref<Lightmapper>(); }

Lightmapper::Lightmapper() {}

Lightmapper::~Lightmapper() {}

void Lightmapper::add_mesh(const MeshData& p_mesh) {}

void Lightmapper::add_directional_light(const String& p_name, bool p_static,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_angular_distance, float p_shadow_blur)
{
}

void Lightmapper::add_omni_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Color& p_color, float p_energy, float p_indirect_energy, float p_range,
	float p_attenuation, float p_size, float p_shadow_blur)
{
}

void Lightmapper::add_spot_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_range, float p_attenuation, float p_spot_angle, float p_spot_attenuation, float p_size,
	float p_shadow_blur)
{
}

void Lightmapper::add_area_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_range, float p_attenuation, const Vector3& p_area_width, const Vector3& p_area_height,
	float p_size, float p_shadow_blur, const Rect2& p_texture_rect, float p_max_mipmap)
{
}

void Lightmapper::add_area_light_atlas(
	const Vector2i& p_size, int p_mipmap_count, const PackedByteArray& p_atlas_data)
{
}

void Lightmapper::add_probe(const Vector3& p_position) {}

Lightmapper::BakeError Lightmapper::bake(BakeQuality p_quality, bool p_use_denoiser,
	float p_denoiser_strength, int p_denoiser_range, int p_bounces, float p_bounce_indirect_energy,
	float p_bias, int p_max_texture_size, bool p_bake_sh, bool p_bake_shadowmask,
	bool p_texture_for_bounces, GenerateProbes p_generate_probes,
	const Ref<Image>& p_environment_panorama, const Basis& p_environment_transform,
	BakeStepFunc p_step_function, void* p_step_userdata, float p_exposure_normalization,
	float p_supersampling_factor)
{
	return BAKE_ERROR_LIGHTMAP_CANT_PRE_BAKE_MESHES;
}

int Lightmapper::get_bake_texture_count() const { return 0; }

Ref<Image> Lightmapper::get_bake_texture(int p_index) const { return Ref<Image>(); }

int Lightmapper::get_shadowmask_texture_count() const { return 0; }

Ref<Image> Lightmapper::get_shadowmask_texture(int p_index) const { return Ref<Image>(); }

int Lightmapper::get_bake_mesh_count() const { return 0; }

Rect2 Lightmapper::get_bake_mesh_uv_scale(int p_index) const { return Rect2(); }

int Lightmapper::get_bake_mesh_texture_slice(int p_index) const { return 0; }

int Lightmapper::get_bake_probe_count() const { return 0; }

Vector3 Lightmapper::get_bake_probe_point(int p_probe) const { return Vector3(); }

Vector<Color> Lightmapper::get_bake_probe_sh(int p_probe) const { return Vector<Color>(); }

#endif // MODULE_LIGHTMAPPER_RD_ENABLED
