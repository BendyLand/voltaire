/**************************************************************************/
/*  sky.cpp                                                               */
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

#include "servers/rendering/renderer.h"
#include "sky.h"

void Sky::set_radiance_size(RadianceSize p_size)
{
	ERR_FAIL_INDEX(p_size, RADIANCE_SIZE_MAX);

	radiance_size = p_size;
	static const int size[RADIANCE_SIZE_MAX] = {32, 64, 128, 256, 512, 1024, 2048};
	Renderer::sky_set_radiance_size(sky, size[radiance_size]);
}

Sky::RadianceSize Sky::get_radiance_size() const { return radiance_size; }

void Sky::set_process_mode(ProcessMode p_mode)
{
	mode = p_mode;
	Renderer::sky_set_mode(sky, RSE::SkyMode(mode));
}

Sky::ProcessMode Sky::get_process_mode() const { return mode; }

void Sky::set_material(const Ref<Material>& p_material)
{
	sky_material = p_material;
	RID material_rid;
	if (sky_material.is_valid()) {
		material_rid = sky_material->get_rid();
	}
	Renderer::sky_set_material(sky, material_rid);
}

Ref<Material> Sky::get_material() const { return sky_material; }

RID Sky::get_rid() const { return sky; }


Sky::Sky() { sky = Renderer::sky_create(); }

Sky::~Sky()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(sky);
}


