/**************************************************************************/
/*  register_server_types.cpp                                             */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "register_server_types.h"
#include "servers/audio/audio_effect.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"
#include "servers/camera/camera_feed.h"
#include "servers/camera/camera_server.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/display/native_menu.h"
#include "servers/movie_writer/movie_writer.h"
#include "servers/movie_writer/movie_writer_pngwav.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/storage_rd/render_data_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_data_rd.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/shader_include_db.h"
#include "servers/rendering/shader_types.h"
#include "servers/rendering/storage/render_scene_buffers.h"
#include "servers/text/text_server.h"
#include "servers/text/text_server_dummy.h"

// 2D physics and navigation.
#ifndef NAVIGATION_2D_DISABLED
#include "servers/navigation_2d/navigation_server_2d.h"
#endif // NAVIGATION_2D_DISABLED
#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_2D_DISABLED

// 3D physics and navigation.
#ifndef NAVIGATION_3D_DISABLED
#include "servers/navigation_3d/navigation_server_3d.h"
#endif // NAVIGATION_3D_DISABLED
#ifndef PHYSICS_3D_DISABLED
#include "servers/physics_3d/physics_server_3d.h"
#endif // PHYSICS_3D_DISABLED

// XR
#ifndef XR_DISABLED
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_server.h"
#include "servers/xr/xr_tracker.h"
#endif // XR_DISABLED

static bool has_server_feature_callback(const String& p_feature)
{
	if (Renderer::data) {
		if (Renderer::has_os_feature(p_feature)) {
			return true;
		}
	}

	return false;
}

static MovieWriterPNGWAV* writer_pngwav = nullptr;

void unregister_server_types()
{
	ShaderTypes::finalize();
}

void register_server_types()
{
	ShaderTypes::initialize();
}


