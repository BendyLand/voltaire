/**************************************************************************/
/*  openxr_visibility_mask_extension.cpp                                  */
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

#include "../openxr_api.h"
#include "core/string/print_string.h"
#include "openxr_visibility_mask_extension.h"
#include "servers/rendering/renderer.h"

void OpenXRVisibilityMaskExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRVisibilityMaskExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}


static const char* VISIBILITY_MASK_SHADER_CODE =
	"shader_type spatial;\n"
	"render_mode unshaded, shadows_disabled, cull_disabled;\n"
	"void vertex() {\n"
	"\tif (int(VERTEX.z) == VIEW_INDEX) {\n"
	"\t\tVERTEX.z = -1.0;\n"
	"\t\tVERTEX += EYE_OFFSET;\n"
	"\t\tPOSITION = PROJECTION_MATRIX * vec4(VERTEX, 1.0);\n"
	"\t\tPOSITION.xy /= POSITION.w;\n"
	"\t\tPOSITION.z = 1.0;\n"
	"\t\tPOSITION.w = 1.0;\n"
	"\t} else {\n"
	"\t\tPOSITION = vec4(2.0, 2.0, 2.0, 1.0);\n"
	"\t}\n"
	"}\n"
	"void fragment() {\n"
	"\tALBEDO = vec3(0.0, 0.0, 0.0);\n"
	"}\n";

HashMap<String, bool*> OpenXRVisibilityMaskExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_KHR_VISIBILITY_MASK_EXTENSION_NAME] = &data->available;

	return request_extensions;
}

void OpenXRVisibilityMaskExtension::on_instance_created(const XrInstance p_instance)
{
	if (data->available) {
		EXT_INIT_XR_FUNC(xrGetVisibilityMaskKHR);
	}
}

void OpenXRVisibilityMaskExtension::on_session_created(const XrSession p_instance)
{
	if (data->available) {
		ERR_FAIL_NULL(Renderer::data);

		ERR_FAIL_COND(!OpenXRAPI::is_allocated());

		// Create our shader.
		data->shader = Renderer::shader_create();
		Renderer::shader_set_code(data->shader, VISIBILITY_MASK_SHADER_CODE);

		// Create our material.
		data->material = Renderer::material_create();
		Renderer::material_set_shader(data->material, data->shader);
		Renderer::material_set_render_priority(data->material, 99);

		// Get our initial mesh data.
		data->mesh_count = OpenXRAPI::get_view_count(); // We need a mesh for each view.
		for (uint32_t i = 0; i < data->mesh_count; i++) {
			_update_mesh_data(i);
		}

		// And update our mesh
		_update_mesh();
	}
}

void OpenXRVisibilityMaskExtension::on_session_destroyed()
{
	ERR_FAIL_NULL(Renderer::data);

	// Free our mesh.
	if (data->mesh.is_valid()) {
		Renderer::free_rid(data->mesh);
		data->mesh = RID();
	}

	// Free our material.
	if (data->material.is_valid()) {
		Renderer::free_rid(data->material);
		data->material = RID();
	}

	// Free our shader.
	if (data->shader.is_valid()) {
		Renderer::free_rid(data->shader);
		data->shader = RID();
	}

	data->mesh_count = 0;
}

void OpenXRVisibilityMaskExtension::on_pre_render()
{
	// Update mesh data if its dirty.
	// Here we call this from the rendering thread however as we're going through the rendering
	// server this is safe.
	_update_mesh();
}

bool OpenXRVisibilityMaskExtension::on_event_polled(const XrEventDataBuffer& event)
{
	if (event.type == XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR) {
		XrEventDataVisibilityMaskChangedKHR* vismask_event =
			(XrEventDataVisibilityMaskChangedKHR*)&event;

		print_verbose("OpenXR EVENT: Visibility mask changed for view " +
					  String::num_uint64(vismask_event->viewIndex));

		if (data->available) { // This event won't be called if this extension is not available but better
						 // safe than sorry.
			_update_mesh_data(vismask_event->viewIndex);
		}

		return true;
	}

	return false;
}

bool OpenXRVisibilityMaskExtension::is_available() { return data != nullptr && data->available; }

RID OpenXRVisibilityMaskExtension::get_mesh() { return data ? data->mesh : RID(); }

void OpenXRVisibilityMaskExtension::_update_mesh_data(uint32_t p_view)
{
	if (data->available) {
		ERR_FAIL_UNSIGNED_INDEX(p_view, 4);

		ERR_FAIL_COND(!OpenXRAPI::is_allocated());

		XrSession session = OpenXRAPI::get_session();
		XrViewConfigurationType view_configuration_type = OpenXRAPI::get_view_configuration();

		// Figure out how much data we're getting.
		XrVisibilityMaskKHR visibility_mask_data = {
			XR_TYPE_VISIBILITY_MASK_KHR,
			nullptr,
			0,
			0,
			nullptr,
			0,
			0,
			nullptr,
		};

		XrResult result = xrGetVisibilityMaskKHR(session, view_configuration_type, p_view,
			XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR, &visibility_mask_data);
		if (XR_FAILED(result)) {
			// print_line("OpenXR: Unable to obtain visibility mask metrics [",
			// OpenXRAPI::get_error_string(result), "]");
			return;
		}

		// Resize buffers
		data->mesh_data[p_view].vertices.resize(visibility_mask_data.vertexCountOutput);
		data->mesh_data[p_view].indices.resize(visibility_mask_data.indexCountOutput);

		visibility_mask_data.vertexCapacityInput = visibility_mask_data.vertexCountOutput;
		visibility_mask_data.vertices = data->mesh_data[p_view].vertices.ptrw();
		visibility_mask_data.indexCapacityInput = visibility_mask_data.indexCountOutput;
		visibility_mask_data.indices = data->mesh_data[p_view].indices.ptrw();

		result = xrGetVisibilityMaskKHR(session, view_configuration_type, p_view,
			XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR, &visibility_mask_data);
		if (XR_FAILED(result)) {
			// print_line("OpenXR: Unable to obtain visibility mask data [",
			// OpenXRAPI::get_error_string(result), "]");
			return;
		}

		// Mark as dirty, we have updated mesh data.
		data->is_dirty = true;
	}
}

void OpenXRVisibilityMaskExtension::_update_mesh() {}
