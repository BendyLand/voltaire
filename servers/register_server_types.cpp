/**************************************************************************/
/*  register_server_types.cpp                                             */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "register_server_types.h"
#include "servers/audio/audio_effect.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"
#include "servers/audio/effects/audio_effect_amplify.h"
#include "servers/audio/effects/audio_effect_capture.h"
#include "servers/audio/effects/audio_effect_chorus.h"
#include "servers/audio/effects/audio_effect_compressor.h"
#include "servers/audio/effects/audio_effect_delay.h"
#include "servers/audio/effects/audio_effect_distortion.h"
#include "servers/audio/effects/audio_effect_eq.h"
#include "servers/audio/effects/audio_effect_filter.h"
#include "servers/audio/effects/audio_effect_hard_limiter.h"
#include "servers/audio/effects/audio_effect_panner.h"
#include "servers/audio/effects/audio_effect_phaser.h"
#include "servers/audio/effects/audio_effect_pitch_shift.h"
#include "servers/audio/effects/audio_effect_record.h"
#include "servers/audio/effects/audio_effect_reverb.h"
#include "servers/audio/effects/audio_effect_spectrum_analyzer.h"
#include "servers/audio/effects/audio_effect_stereo_enhance.h"
#include "servers/audio/effects/audio_stream_generator.h"
#include "servers/camera/camera_feed.h"
#include "servers/camera/camera_server.h"
#include "servers/debugger/servers_debugger.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/display/native_menu.h"
#include "servers/movie_writer/movie_writer.h"
#include "servers/movie_writer/movie_writer_pngwav.h"
#include "servers/rendering/renderer_rd/framebuffer_cache_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_data_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_data_rd.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering/rendering_server.h"
#include "servers/rendering/shader_include_db.h"
#include "servers/rendering/shader_types.h"
#include "servers/rendering/storage/render_data.h"
#include "servers/rendering/storage/render_scene_buffers.h"
#include "servers/rendering/storage/render_scene_data.h"
#include "servers/text/text_server.h"
#include "servers/text/text_server_dummy.h"

#ifndef DISABLE_DEPRECATED
#include "servers/audio/effects/audio_effect_limiter.h"
#endif

// 2D physics and navigation.
#ifndef NAVIGATION_2D_DISABLED
#include "servers/navigation_2d/navigation_server_2d.h"
#endif // NAVIGATION_2D_DISABLED
#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#include "servers/physics_2d/physics_server_2d_dummy.h"
#endif // PHYSICS_2D_DISABLED

// 3D physics and navigation.
#ifndef NAVIGATION_3D_DISABLED
#include "servers/navigation_3d/navigation_server_3d.h"
#endif // NAVIGATION_3D_DISABLED
#ifndef PHYSICS_3D_DISABLED
#include "servers/physics_3d/physics_server_3d.h"
#include "servers/physics_3d/physics_server_3d_dummy.h"
#endif // PHYSICS_3D_DISABLED

// XR
#ifndef XR_DISABLED
#include "servers/xr/xr_body_tracker.h"
#include "servers/xr/xr_controller_tracker.h"
#include "servers/xr/xr_face_tracker.h"
#include "servers/xr/xr_hand_tracker.h"
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_positional_tracker.h"
#include "servers/xr/xr_server.h"
#include "servers/xr/xr_vrs.h"
#endif // XR_DISABLED

ShaderTypes* shader_types = nullptr;

#ifndef PHYSICS_2D_DISABLED
static PhysicsServer2D* _create_dummy_physics_server_2d() { return memnew(PhysicsServer2DDummy); }
#endif // PHYSICS_2D_DISABLED

#ifndef PHYSICS_3D_DISABLED
static PhysicsServer3D* _create_dummy_physics_server_3d() { return memnew(PhysicsServer3DDummy); }
#endif // PHYSICS_3D_DISABLED

static bool has_server_feature_callback(const String& p_feature)
{
	if (RenderingServer::get_singleton()) {
		if (RenderingServer::get_singleton()->has_os_feature(p_feature)) {
			return true;
		}
	}

	return false;
}

static MovieWriterPNGWAV* writer_pngwav = nullptr;

void register_server_types()
{
	OS::get_singleton()->benchmark_begin_measure("Servers", "Register Extensions");

	shader_types = memnew(ShaderTypes);

	Engine::get_singleton()->add_singleton(Engine::Singleton(
		"TextServerManager", TextServerManager::get_singleton(), "TextServerManager"));

	OS::get_singleton()->set_has_server_feature_callback(has_server_feature_callback);

	{
	// audio effects

#ifndef DISABLE_DEPRECATED
#endif
	}

	ServersDebugger::initialize();

#ifndef NAVIGATION_2D_DISABLED
	Engine::get_singleton()->add_singleton(Engine::Singleton("NavigationServer2DManager",
		NavigationServer2DManager::get_singleton(), "NavigationServer2DManager"));

	GLOBAL_DEF(PropertyInfo(Variant::STRING, NavigationServer2DManager::setting_property_name,
				   PROPERTY_HINT_ENUM, "DEFAULT"),
		"DEFAULT");

	NavigationServer2DManager::get_singleton()->register_server(
		"Dummy", callable_mp_static(NavigationServer2DManager::create_dummy_server_callback));
#endif // NAVIGATION_2D_DISABLED

#ifndef PHYSICS_2D_DISABLED
	// Physics 2D
	Engine::get_singleton()->add_singleton(Engine::Singleton("PhysicsServer2DManager",
		PhysicsServer2DManager::get_singleton(), "PhysicsServer2DManager"));

	GLOBAL_DEF(PropertyInfo(Variant::STRING, PhysicsServer2DManager::setting_property_name,
				   PROPERTY_HINT_ENUM, "DEFAULT"),
		"DEFAULT");

	PhysicsServer2DManager::get_singleton()->register_server(
		"Dummy", callable_mp_static(_create_dummy_physics_server_2d));
#endif // PHYSICS_2D_DISABLED

#ifndef NAVIGATION_3D_DISABLED
	Engine::get_singleton()->add_singleton(Engine::Singleton("NavigationServer3DManager",
		NavigationServer3DManager::get_singleton(), "NavigationServer3DManager"));

	GLOBAL_DEF(PropertyInfo(Variant::STRING, NavigationServer3DManager::setting_property_name,
				   PROPERTY_HINT_ENUM, "DEFAULT"),
		"DEFAULT");

	NavigationServer3DManager::get_singleton()->register_server(
		"Dummy", callable_mp_static(NavigationServer3DManager::create_dummy_server_callback));
#endif // NAVIGATION_3D_DISABLED

#ifndef PHYSICS_3D_DISABLED
	// Physics 3D
	Engine::get_singleton()->add_singleton(Engine::Singleton("PhysicsServer3DManager",
		PhysicsServer3DManager::get_singleton(), "PhysicsServer3DManager"));

	GLOBAL_DEF(PropertyInfo(Variant::STRING, PhysicsServer3DManager::setting_property_name,
				   PROPERTY_HINT_ENUM, "DEFAULT"),
		"DEFAULT");

	PhysicsServer3DManager::get_singleton()->register_server(
		"Dummy", callable_mp_static(_create_dummy_physics_server_3d));
#endif // PHYSICS_3D_DISABLED

#ifndef XR_DISABLED
#endif // XR_DISABLED

	if constexpr (VLTR_IS_CLASS_ENABLED(MovieWriterPNGWAV)) {
		writer_pngwav = memnew(MovieWriterPNGWAV);
		MovieWriter::add_writer(writer_pngwav);
	}

	OS::get_singleton()->benchmark_end_measure("Servers", "Register Extensions");
}

void unregister_server_types()
{
	OS::get_singleton()->benchmark_begin_measure("Servers", "Unregister Extensions");

	ServersDebugger::deinitialize();
	memdelete(shader_types);
	if constexpr (VLTR_IS_CLASS_ENABLED(MovieWriterPNGWAV)) {
		memdelete(writer_pngwav);
	}

	OS::get_singleton()->benchmark_end_measure("Servers", "Unregister Extensions");
}

void register_server_singletons()
{
	OS::get_singleton()->benchmark_begin_measure("Servers", "Register Singletons");

	Engine::get_singleton()->add_singleton(Engine::Singleton(
		"AccessibilityServer", AccessibilityServer::get_singleton(), "AccessibilityServer"));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("AudioServer", AudioServer::get_singleton(), "AudioServer"));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("CameraServer", CameraServer::get_singleton(), "CameraServer"));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("DisplayServer", DisplayServer::get_singleton(), "DisplayServer"));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("NativeMenu", NativeMenu::get_singleton(), "NativeMenu"));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("RenderingServer", RenderingServer::get_singleton(), "RenderingServer"));
#ifndef NAVIGATION_2D_DISABLED
	Engine::get_singleton()->add_singleton(Engine::Singleton(
		"NavigationServer2D", NavigationServer2D::get_singleton(), "NavigationServer2D"));
#endif // NAVIGATION_2D_DISABLED
#ifndef NAVIGATION_3D_DISABLED
	Engine::get_singleton()->add_singleton(Engine::Singleton(
		"NavigationServer3D", NavigationServer3D::get_singleton(), "NavigationServer3D"));
#endif // NAVIGATION_3D_DISABLED
#ifndef PHYSICS_2D_DISABLED
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("PhysicsServer2D", PhysicsServer2D::get_singleton(), "PhysicsServer2D"));
#endif // PHYSICS_2D_DISABLED
#ifndef PHYSICS_3D_DISABLED
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("PhysicsServer3D", PhysicsServer3D::get_singleton(), "PhysicsServer3D"));
#endif // PHYSICS_3D_DISABLED
#ifndef XR_DISABLED
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("XRServer", XRServer::get_singleton(), "XRServer"));
#endif // XR_DISABLED

	OS::get_singleton()->benchmark_end_measure("Servers", "Register Singletons");
}


