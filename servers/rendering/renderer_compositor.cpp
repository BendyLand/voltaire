/**************************************************************************/
/*  renderer_compositor.cpp                                               */
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
/* without limitation the rights to use, copy, modify, merge, publish,   */
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

#include "renderer_compositor.h"
#include "servers/display/display_server.h"

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

Error RendererCompositor::create()
{
    if (_create_func != nullptr) {
        return _create_func();
    }
#if defined(RD_ENABLED)
    low_end = false;
    bind_compositor<RendererCompositorRD>();
    DisplayServer* ds = DisplayServer::get_singleton();
    if (ds != nullptr) {
        Error err = ds->init_rendering_device();
        ERR_FAIL_COND_V_MSG(err != OK, err, "Failed to initialize RenderingDevice via DisplayServer.");
    }
    RendererCompositorRD::initialize();
    return OK;
#elif defined(GLES3_ENABLED)
    low_end = true;
    bind_compositor<RasterizerGLES3>();
    RasterizerGLES3::initialize();
    return OK;
#else
    low_end = true;
    bind_compositor<RasterizerDummy>();
    RasterizerDummy::initialize();
    return OK;
#endif
}

bool RendererCompositor::is_xr_enabled()
{
#ifndef XR_DISABLED
    return xr_enabled;
#else
    return false;
#endif
}
