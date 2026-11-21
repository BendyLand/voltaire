/**************************************************************************/
/*  xr_server.cpp                                                         */
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

#include "core/config/project_settings.h"
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_tracker.h"
#include "xr_server.compat.inc"
#include "xr_server.h"

XRServer::XRMode XRServer::xr_mode = XRMODE_DEFAULT;

void XRServer::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
}

void XRServer::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

XRServer::XRMode XRServer::get_xr_mode() { return xr_mode; }

void XRServer::set_xr_mode(XRServer::XRMode p_mode) { xr_mode = p_mode; }

double XRServer::get_world_scale()
{
	ERR_FAIL_NULL_V(data, 1.0);
	if (Renderer::data && Renderer::is_on_render_thread()) {
		// Return the value with which we're currently rendering,
		// if we're on the render thread
		return data->render_state.world_scale;
	}
	else {
		// Return our current value
		return data->world_scale;
	}
}

void XRServer::set_world_scale(double p_world_scale)
{
	ERR_FAIL_NULL(data);
	if (p_world_scale < 0.01) {
		p_world_scale = 0.01;
	}
	else if (p_world_scale > 1000.0) {
		p_world_scale = 1000.0;
	}

	data->world_scale = p_world_scale;
	set_render_world_scale(data->world_scale);
}

void XRServer::_set_render_world_scale(double p_world_scale)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_NULL(data);
	data->render_state.world_scale = p_world_scale;
}

void XRServer::_set_render_world_origin(const Transform3D& p_world_origin)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_NULL(data);
	data->render_state.world_origin = p_world_origin;
}

void XRServer::_set_render_reference_frame(const Transform3D& p_reference_frame)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_NULL(data);
	data->render_state.reference_frame = p_reference_frame;
}

Transform3D XRServer::get_reference_frame() { return Transform3D(); }

void XRServer::set_camera_locked_to_origin(bool p_enable)
{
	ERR_FAIL_NULL(data);
	data->camera_locked_to_origin = p_enable;
}

Ref<XRInterface> XRServer::get_primary_interface()
{
	Ref<XRInterface> xri = memnew(XRInterface);
	return xri;
}

void XRServer::remove_tracker(const Ref<XRTracker>& p_tracker) {}

void XRServer::set_primary_interface(const Ref<XRInterface>& p_primary_interface) {}

void XRServer::add_tracker(const Ref<XRTracker>& p_tracker) {}

Ref<XRTracker> XRServer::get_tracker(const String& p_name)
{
	Ref<XRTracker> xpt = memnew(XRTracker);
	return xpt;
}

void XRServer::set_render_world_scale(double p_scale) {}

void XRServer::remove_interface(const Ref<XRInterface>& p_interface) {}

Transform3D XRServer::get_world_origin() { return Transform3D(); }

void XRServer::set_world_origin(const Transform3D& p_world_origin) {}

void XRServer::_process() {}

void XRServer::clear_reference_frame() {}

void XRServer::add_interface(const Ref<XRInterface>& p_interface) {}
