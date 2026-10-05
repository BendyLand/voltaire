/**************************************************************************/
/*  renderer_compositor.h                                                 */
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

#pragma once

#include "core/io/image.h"
#include "servers/display/display_server_enums.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/utilities.h"

class RendererCanvasRender;
class RendererSceneRender;

class RendererFog;
class RendererGI;
class RendererLightStorage;
class RendererMaterialStorage;
class RendererMeshStorage;
class RendererParticlesStorage;
class RendererTextureStorage;
class RendererUtilities;

class RendererCompositor final
{
private:
	static inline bool xr_enabled = false;

public:
	static inline Error (*_create_func)() = nullptr;
	static inline bool back_end = false;
	static inline bool low_end = false;

	static Error create();
	static bool is_xr_enabled();

	_ALWAYS_INLINE_ static bool is_low_end() { return low_end; }

	/* Static Function Pointer Dispatch Interface */

	static inline RendererLightStorage* (*get_light_storage)() = nullptr;
	static inline RendererMaterialStorage* (*get_material_storage)() = nullptr;
	static inline RendererMeshStorage* (*get_mesh_storage)() = nullptr;
	static inline RendererParticlesStorage* (*get_particles_storage)() = nullptr;
	static inline RendererTextureStorage* (*get_texture_storage)() = nullptr;
	static inline RendererGI* (*get_gi)() = nullptr;
	static inline RendererFog* (*get_fog)() = nullptr;
	static inline RendererCanvasRender* (*get_canvas)() = nullptr;

	static inline void (*set_boot_image_with_stretch)(
		const Ref<Image>&, const Color&, RSE::SplashStretchMode, bool) = nullptr;
	static inline void (*initialize)() = nullptr;
	static inline bool (*is_initialized)() = nullptr;
	static inline void (*begin_frame)(double) = nullptr;
	static inline void (*blit_render_targets_to_screen)(
		DisplayServerEnums::WindowID, const RenderingServerTypes::BlitToScreen*, int) = nullptr;

	static inline bool (*is_opengl)() = nullptr;
	static inline void (*gl_end_frame)(bool) = nullptr;
	static inline void (*end_frame)(bool) = nullptr;
	static inline void (*finalize)() = nullptr;

	static inline uint64_t (*get_frame_number)() = nullptr;
	static inline double (*get_frame_delta_time)() = nullptr;
	static inline double (*get_total_time)() = nullptr;
	static inline bool (*can_create_resources_async)() = nullptr;

	template <typename Backend> static void bind_compositor()
	{
		Backend::bind_utilities();

		get_light_storage = &Backend::get_light_storage;
		get_material_storage = &Backend::get_material_storage;
		// get_mesh_storage = &Backend::get_mesh_storage;
		Backend::bind_mesh_storage();
		get_particles_storage = &Backend::get_particles_storage;
		get_texture_storage = &Backend::get_texture_storage;
		get_gi = &Backend::get_gi;
		get_fog = &Backend::get_fog;
		get_canvas = &Backend::get_canvas;

		set_boot_image_with_stretch = &Backend::set_boot_image_with_stretch;
		initialize = &Backend::initialize;
		begin_frame = &Backend::begin_frame;
		blit_render_targets_to_screen = &Backend::blit_render_targets_to_screen;

		is_opengl = &Backend::is_opengl;
		gl_end_frame = &Backend::gl_end_frame;
		end_frame = &Backend::end_frame;
		finalize = &Backend::finalize;

		get_frame_number = &Backend::get_frame_number;
		get_frame_delta_time = &Backend::get_frame_delta_time;
		get_total_time = &Backend::get_total_time;
		can_create_resources_async = &Backend::can_create_resources_async;
	}

	RendererCompositor() = delete;
	RendererCompositor(const RendererCompositor&) = delete;
	~RendererCompositor() = delete;
};

using RC = RendererCompositor;


