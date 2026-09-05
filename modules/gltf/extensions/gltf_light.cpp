/**************************************************************************/
/*  gltf_light.cpp                                                        */
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

#include <cfloat> // FLT_MAX
#include "../structures/gltf_object_model_property.h"
#include "gltf_light.h"
#include "scene/3d/light_3d.h"

void GLTFLight::_bind_methods() {}

void GLTFLight::set_cone_inner_attenuation_conversion_expressions(
	Ref<GLTFObjectModelProperty>& r_obj_model_prop)
{
	// Expression to convert glTF innerConeAngle to Godot spot_angle_attenuation.
	Ref<Expression> gltf_to_godot_expr;
	gltf_to_godot_expr.instantiate();
	PackedStringArray gltf_to_godot_args = {"inner_cone_angle"};
	gltf_to_godot_expr->parse(
		"0.2 / (1.0 - inner_cone_angle / spot_angle) - 0.1", gltf_to_godot_args);
	r_obj_model_prop->set_gltf_to_godot_expression(gltf_to_godot_expr);
	// Expression to convert Godot spot_angle_attenuation to glTF innerConeAngle.
	Ref<Expression> godot_to_gltf_expr;
	godot_to_gltf_expr.instantiate();
	PackedStringArray godot_to_gltf_args = {"godot_spot_angle_att"};
	godot_to_gltf_expr->parse(
		"spot_angle * maxf(0.0, 1.0 - (0.2 / (0.1 + godot_spot_angle_att)))", godot_to_gltf_args);
	r_obj_model_prop->set_godot_to_gltf_expression(godot_to_gltf_expr);
}

Color GLTFLight::get_color() { return color; }

void GLTFLight::set_color(Color p_color) { color = p_color; }

float GLTFLight::get_intensity() { return intensity; }

void GLTFLight::set_intensity(float p_intensity) { intensity = p_intensity; }

String GLTFLight::get_light_type() { return light_type; }

void GLTFLight::set_light_type(String p_light_type) { light_type = p_light_type; }

float GLTFLight::get_range() { return range; }

void GLTFLight::set_range(float p_range) { range = p_range; }

float GLTFLight::get_inner_cone_angle() { return inner_cone_angle; }

void GLTFLight::set_inner_cone_angle(float p_inner_cone_angle)
{
	inner_cone_angle = p_inner_cone_angle;
}

float GLTFLight::get_outer_cone_angle() { return outer_cone_angle; }

void GLTFLight::set_outer_cone_angle(float p_outer_cone_angle)
{
	outer_cone_angle = p_outer_cone_angle;
}

Light3D* GLTFLight::to_node() const
{
	Light3D* light = nullptr;
	if (light_type == "directional") {
		DirectionalLight3D* dir_light = memnew(DirectionalLight3D);
		dir_light->set_param(Light3D::PARAM_ENERGY, intensity);
		light = dir_light;
	}
	else if (light_type == "point") {
		OmniLight3D* omni_light = memnew(OmniLight3D);
		omni_light->set_param(OmniLight3D::PARAM_ENERGY, intensity);
		omni_light->set_param(OmniLight3D::PARAM_RANGE, CLAMP(range, 0, 4096));
		light = omni_light;
	}
	else if (light_type == "spot") {
		SpotLight3D* spot_light = memnew(SpotLight3D);
		spot_light->set_param(SpotLight3D::PARAM_ENERGY, intensity);
		spot_light->set_param(SpotLight3D::PARAM_RANGE, CLAMP(range, 0, 4096));
		spot_light->set_param(SpotLight3D::PARAM_SPOT_ANGLE, Math::rad_to_deg(outer_cone_angle));
		// Line of best fit derived from guessing, see https://www.desmos.com/calculator/biiflubp8b
		// The points in desmos are not exact, except for (1, infinity).
		float angle_ratio = inner_cone_angle / outer_cone_angle;
		float angle_attenuation = 0.2 / (1 - angle_ratio) - 0.1;
		spot_light->set_param(SpotLight3D::PARAM_SPOT_ATTENUATION, angle_attenuation);
		light = spot_light;
	}
	else {
		ERR_PRINT("Failed to create a Light3D node from GLTFLight, unknown light type '" +
				  light_type + "'.");
		return nullptr;
	}
	light->set_color(color.linear_to_srgb());
	light->set_param(Light3D::PARAM_ATTENUATION, 2.0);
	return light;
}


