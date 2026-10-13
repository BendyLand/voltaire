/**************************************************************************/
/*  bokeh_dof.cpp                                                         */
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

#include "bokeh_dof.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/storage/camera_attributes_storage.h"

using namespace RendererRD;

BokehDOF::~BokehDOF()
{
	for (int i = 0; i < BOKEH_MAX; i++) {
		bokeh.compute_pipelines[i].free();
	}

	if (prefer_raster_effects) {
		bokeh.raster_shader.version_free(bokeh.shader_version);
	}
	else {
		bokeh.compute_shader.version_free(bokeh.shader_version);
	}
}

void RendererRD::BokehDOF::bokeh_dof_compute(const RendererRD::BokehDOF::BokehBuffers& p_buffers,
	RID p_dest_framebuffer, float p_bokeh_size, float p_blur_size, bool p_use_jitter)
{
}

BokehDOF::BokehDOF(bool p_prefer_raster_effects)
{
	prefer_raster_effects = p_prefer_raster_effects;

	Vector<String> modes;
	modes.push_back("\n#define MODE_GEN_BLUR_SIZE\n");
	modes.push_back("\n#define MODE_BOKEH_BOX\n");
	modes.push_back("\n#define MODE_BOKEH_BOX\n#define MODE_NO_WEIGHTS\n");
	modes.push_back("\n#define MODE_BOKEH_HEXAGONAL\n");
	modes.push_back("\n#define MODE_BOKEH_HEXAGONAL\n#define MODE_NO_WEIGHTS\n");
	modes.push_back("\n#define MODE_BOKEH_CIRCULAR\n");
	modes.push_back("\n#define MODE_COMPOSITE\n");

	if (prefer_raster_effects) {
		bokeh.raster_shader.initialize(modes);
		bokeh.shader_version = bokeh.raster_shader.version_create();
	}
	else {
		bokeh.compute_shader.initialize(modes);
		bokeh.shader_version = bokeh.compute_shader.version_create();
	}
}


