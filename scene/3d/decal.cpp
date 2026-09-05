/**************************************************************************/
/*  decal.cpp                                                             */
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

#include "core/os/os.h"
#include "decal.h"
#include "servers/rendering/rendering_server.h"

void Decal::set_size(const Vector3& p_size)
{
	size = p_size.maxf(0.001);
	RS::get_singleton()->decal_set_size(decal, size);
	update_gizmos();
}

Vector3 Decal::get_size() const { return size; }

void Decal::set_texture(DecalTexture p_type, const Ref<Texture2D>& p_texture)
{
	ERR_FAIL_INDEX(p_type, TEXTURE_MAX);
	textures[p_type] = p_texture;
	RID texture_rid = p_texture.is_valid() ? p_texture->get_rid() : RID();

#ifdef DEBUG_ENABLED
	if (p_texture.is_valid() &&
		(p_texture->obj->is_class("AnimatedTexture") || p_texture->obj->is_class("AtlasTexture") ||
			p_texture->obj->is_class("CameraTexture") ||
			p_texture->obj->is_class("CanvasTexture") || p_texture->obj->is_class("MeshTexture") ||
			p_texture->obj->is_class("Texture2DRD") ||
			p_texture->obj->is_class("ViewportTexture"))) {
		WARN_PRINT(vformat("%s cannot be used as a Decal texture (%s). As a workaround, assign the "
						   "value returned by %s's `get_image()` instead.",
			p_texture->obj->get_class(), get_path(), p_texture->obj->get_class()));
	}
#endif

	RS::get_singleton()->decal_set_texture(decal, RSE::DecalTexture(p_type), texture_rid);
	update_configuration_warnings();
}

Ref<Texture2D> Decal::get_texture(DecalTexture p_type) const
{
	ERR_FAIL_INDEX_V(p_type, TEXTURE_MAX, Ref<Texture2D>());
	return textures[p_type];
}

void Decal::set_emission_energy(real_t p_energy)
{
	emission_energy = p_energy;
	RS::get_singleton()->decal_set_emission_energy(decal, emission_energy);
}

real_t Decal::get_emission_energy() const { return emission_energy; }

void Decal::set_albedo_mix(real_t p_mix)
{
	albedo_mix = p_mix;
	RS::get_singleton()->decal_set_albedo_mix(decal, albedo_mix);
}

real_t Decal::get_albedo_mix() const { return albedo_mix; }

void Decal::set_upper_fade(real_t p_fade)
{
	upper_fade = MAX(p_fade, 0.0);
	RS::get_singleton()->decal_set_fade(decal, upper_fade, lower_fade);
}

real_t Decal::get_upper_fade() const { return upper_fade; }

void Decal::set_lower_fade(real_t p_fade)
{
	lower_fade = MAX(p_fade, 0.0);
	RS::get_singleton()->decal_set_fade(decal, upper_fade, lower_fade);
}

real_t Decal::get_lower_fade() const { return lower_fade; }

void Decal::set_normal_fade(real_t p_fade)
{
	normal_fade = p_fade;
	RS::get_singleton()->decal_set_normal_fade(decal, normal_fade);
}

real_t Decal::get_normal_fade() const { return normal_fade; }

void Decal::set_modulate(Color p_modulate)
{
	modulate = p_modulate;
	RS::get_singleton()->decal_set_modulate(decal, p_modulate);
}

Color Decal::get_modulate() const { return modulate; }

void Decal::set_enable_distance_fade(bool p_enable)
{
	distance_fade_enabled = p_enable;
	RS::get_singleton()->decal_set_distance_fade(
		decal, distance_fade_enabled, distance_fade_begin, distance_fade_length);
	this->obj->notify_property_list_changed();
}

bool Decal::is_distance_fade_enabled() const { return distance_fade_enabled; }

void Decal::set_distance_fade_begin(real_t p_distance)
{
	distance_fade_begin = p_distance;
	RS::get_singleton()->decal_set_distance_fade(
		decal, distance_fade_enabled, distance_fade_begin, distance_fade_length);
}

real_t Decal::get_distance_fade_begin() const { return distance_fade_begin; }

void Decal::set_distance_fade_length(real_t p_length)
{
	distance_fade_length = p_length;
	RS::get_singleton()->decal_set_distance_fade(
		decal, distance_fade_enabled, distance_fade_begin, distance_fade_length);
}

real_t Decal::get_distance_fade_length() const { return distance_fade_length; }

void Decal::set_cull_mask(uint32_t p_layers)
{
	cull_mask = p_layers;
	RS::get_singleton()->decal_set_cull_mask(decal, cull_mask);
	update_configuration_warnings();
}

uint32_t Decal::get_cull_mask() const { return cull_mask; }

AABB Decal::get_aabb() const
{
	AABB aabb;
	aabb.position = -size / 2;
	aabb.size = size;
	return aabb;
}

void Decal::_validate_property(PropertyInfo& p_property) const
{
	if (p_property.name == "sorting_offset") {
		p_property.usage = PROPERTY_USAGE_DEFAULT;
	}
}

PackedStringArray Decal::get_configuration_warnings() const
{
	PackedStringArray warnings = VisualInstance3D::get_configuration_warnings();

	if (OS::get_singleton()->get_current_rendering_method() == "gl_compatibility" ||
		OS::get_singleton()->get_current_rendering_method() == "dummy") {
		warnings.push_back(
			RTR("Decals are only available when using the Forward+ or Mobile renderer."));
		return warnings;
	}

	if (textures[TEXTURE_ALBEDO].is_null() && textures[TEXTURE_NORMAL].is_null() &&
		textures[TEXTURE_ORM].is_null() && textures[TEXTURE_EMISSION].is_null()) {
		warnings.push_back(RTR("The decal has no textures loaded into any of its texture "
							   "properties, and will therefore not be visible."));
	}

	if ((textures[TEXTURE_NORMAL].is_valid() || textures[TEXTURE_ORM].is_valid()) &&
		textures[TEXTURE_ALBEDO].is_null()) {
		warnings.push_back(
			RTR("The decal has a Normal and/or ORM texture, but no Albedo texture is set.\nAn "
				"Albedo texture with an alpha channel is required to blend the normal/ORM maps "
				"onto the underlying surface.\nIf you don't want the Albedo texture to be visible, "
				"set Albedo Mix to 0."));
	}

	if (cull_mask == 0) {
		warnings.push_back(RTR("The decal's Cull Mask has no bits enabled, which means the decal "
							   "will not paint objects on any layer.\nTo resolve this, enable at "
							   "least one bit in the Cull Mask property."));
	}

	return warnings;
}

void Decal::_bind_methods() {}

#ifndef DISABLE_DEPRECATED
bool Decal::_set(const StringName& p_name, const Variant& p_value)
{
	if (p_name == "extents") { // Compatibility with Godot 3.x.
		set_size((Vector3)p_value * 2);
		return true;
	}
	return false;
}

bool Decal::_get(const StringName& p_name, Variant& r_property) const
{
	if (p_name == "extents") { // Compatibility with Godot 3.x.
		r_property = size / 2;
		return true;
	}
	return false;
}
#endif // DISABLE_DEPRECATED

Decal::Decal()
{
	decal = RenderingServer::get_singleton()->decal_create();
	RS::get_singleton()->instance_set_base(get_instance(), decal);
}

Decal::~Decal()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(decal);
}


