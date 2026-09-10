/**************************************************************************/
/*  debug_effects.cpp                                                     */
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

#include "debug_effects.h"
#include "servers/rendering/renderer_rd/storage_rd/light_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "servers/rendering/rendering_server_globals.h"

using namespace RendererRD;

void DebugEffects::_create_frustum_arrays()
{
	if (frustum.vertex_buffer.is_null()) {
		// Create vertex buffer, but don't put data in it yet
		frustum.vertex_buffer =
			RD::get_singleton()->vertex_buffer_create(8 * sizeof(float) * 3, Vector<uint8_t>());

		Vector<RD::VertexAttribute> attributes;
		Vector<RID> buffers;
		RD::VertexAttribute vd;

		vd.location = 0;
		vd.stride = sizeof(float) * 3;
		vd.format = RD::DATA_FORMAT_R32G32B32_SFLOAT;

		attributes.push_back(vd);
		buffers.push_back(frustum.vertex_buffer);

		frustum.vertex_format = RD::get_singleton()->vertex_format_create(attributes);
		frustum.vertex_array =
			RD::get_singleton()->vertex_array_create(8, frustum.vertex_format, buffers);
	}

	if (frustum.index_buffer.is_null()) {
		uint16_t indices[6 * 2 * 3] = {
			// Far
			0, 1, 2, // FLT, FLB, FRT
			1, 3, 2, // FLB, FRB, FRT
			// Near
			4, 6, 5, // NLT, NRT, NLB
			6, 7, 5, // NRT, NRB, NLB
			// Left
			0, 4, 1, // FLT, NLT, FLB
			4, 5, 1, // NLT, NLB, FLB
			// Right
			6, 2, 7, // NRT, FRT, NRB
			2, 3, 7, // FRT, FRB, NRB
			// Top
			0, 2, 4, // FLT, FRT, NLT
			2, 6, 4, // FRT, NRT, NLT
			// Bottom
			5, 7, 1, // NLB, NRB, FLB,
			7, 3, 1	 // NRB, FRB, FLB
		};

		// Create our index_array
		PackedByteArray data;
		data.resize(6 * 2 * 3 * 2);
		{
			uint8_t* w = data.ptrw();
			uint16_t* p16 = (uint16_t*)w;
			for (int i = 0; i < 6 * 2 * 3; i++) {
				*p16 = indices[i];
				p16++;
			}
		}
		frustum.index_array =
			RD::get_singleton()->index_array_create(frustum.index_buffer, 0, 6 * 2 * 3);
	}

	if (frustum.lines_buffer.is_null()) {
		uint16_t indices[12 * 2] = {
			0, 1, // FLT - FLB
			1, 3, // FLB - FRB
			3, 2, // FRB - FRT
			2, 0, // FRT - FLT

			4, 6, // NLT - NRT
			6, 7, // NRT - NRB
			7, 5, // NRB - NLB
			5, 4, // NLB - NLT

			0, 4, // FLT - NLT
			1, 5, // FLB - NLB
			2, 6, // FRT - NRT
			3, 7  // FRB - NRB
		};

		// Create our lines_array
		PackedByteArray data;
		data.resize(12 * 2 * 2);
		{
			uint8_t* w = data.ptrw();
			uint16_t* p16 = (uint16_t*)w;
			for (int i = 0; i < 12 * 2; i++) {
				*p16 = indices[i];
				p16++;
			}
		}

		frustum.lines_array =
			RD::get_singleton()->index_array_create(frustum.lines_buffer, 0, 12 * 2);
	}
}

DebugEffects::~DebugEffects()
{
	shadow_frustum.shader.version_free(shadow_frustum.shader_version);

	// Destroy vertex buffer and array.
	if (frustum.vertex_buffer.is_valid()) {
		RD::get_singleton()->free_rid(frustum.vertex_buffer); // Array gets freed as dependency.
	}

	// Destroy index buffer and array,
	if (frustum.index_buffer.is_valid()) {
		RD::get_singleton()->free_rid(frustum.index_buffer); // Array gets freed as dependency.
	}

	// Destroy lines buffer and array.
	if (frustum.lines_buffer.is_valid()) {
		RD::get_singleton()->free_rid(frustum.lines_buffer); // Array gets freed as dependency.
	}

	motion_vectors.shader.version_free(motion_vectors.shader_version);
}


