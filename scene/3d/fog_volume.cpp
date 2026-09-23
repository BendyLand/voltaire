/**************************************************************************/
/*  fog_volume.cpp                                                        */
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
#include "fog_volume.h"
#include "scene/main/viewport.h"
#include "scene/resources/environment.h"
#include "servers/rendering/rendering_server.h"

Vector3 FogVolume::get_size() const { return size; }

RSE::FogVolumeShape FogVolume::get_shape() const { return shape; }

Ref<Material> FogVolume::get_material() const { return material; }

AABB FogVolume::get_aabb() const
{
	if (shape != RSE::FOG_VOLUME_SHAPE_WORLD) {
		return AABB(-size / 2, size);
	}
	return AABB();
}

PackedStringArray FogVolume::get_configuration_warnings() const
{
	PackedStringArray warnings = VisualInstance3D::get_configuration_warnings();

	Ref<Environment> environment = get_viewport()->find_world_3d()->get_environment();

	if (OS::get_singleton()->get_current_rendering_method() != "forward_plus") {
		warnings.push_back(RTR("Fog Volumes are only visible when using the Forward+ renderer."));
		return warnings;
	}

	if (environment.is_valid() && !environment->is_volumetric_fog_enabled()) {
		warnings.push_back(RTR("Fog Volumes need volumetric fog to be enabled in the scene's "
							   "Environment in order to be visible."));
	}

	return warnings;
}

FogVolume::FogVolume()
{
	volume = RS::fog_volume_create();
	RS::fog_volume_set_shape(volume, RSE::FOG_VOLUME_SHAPE_BOX);
	set_base(volume);
}

FogVolume::~FogVolume()
{
	ERR_FAIL_NULL(RenderingServer::data);
	RS::free_rid(volume);
}


