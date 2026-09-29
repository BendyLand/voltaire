/**************************************************************************/
/*  rasterizer_dummy.h                                                    */
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

#pragma once

#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/rendering_server_enums.h"

class RasterizerCanvasDummy;
class RasterizerSceneDummy;

namespace RendererDummy
{
class Fog;
class GI;
class LightStorage;
class MaterialStorage;
class MeshStorage;
class ParticlesStorage;
class TextureStorage;
class Utilities;
} // namespace RendererDummy

class RasterizerDummy final
{
private:
    static inline uint64_t frame = 1;
    static inline double delta = 0.0;
    static inline double time = 0.0;

    static inline RasterizerCanvasDummy* canvas = nullptr;
    static inline RasterizerSceneDummy* scene = nullptr;

    static inline RendererDummy::Fog* fog = nullptr;
    static inline RendererDummy::GI* gi = nullptr;
    static inline RendererDummy::LightStorage* light_storage = nullptr;
    static inline RendererDummy::MaterialStorage* material_storage = nullptr;
    static inline RendererDummy::MeshStorage* mesh_storage = nullptr;
    static inline RendererDummy::ParticlesStorage* particles_storage = nullptr;
    static inline RendererDummy::TextureStorage* texture_storage = nullptr;
    static inline RendererDummy::Utilities* utilities = nullptr;

public:
    static RendererCanvasRender* get_canvas();
    static RendererSceneRender* get_scene();

    static RendererFog* get_fog();
    static RendererGI* get_gi();
    static RendererLightStorage* get_light_storage();
    static RendererMaterialStorage* get_material_storage();
    static RendererMeshStorage* get_mesh_storage();
    static RendererParticlesStorage* get_particles_storage();
    static RendererTextureStorage* get_texture_storage();
    static RendererUtilities* get_utilities();

    static void set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
        RSE::SplashStretchMode p_stretch_mode, bool p_use_filter = true)
    {
    }

    static void initialize() {}

    static void begin_frame(double frame_step)
    {
        frame++;
        delta = frame_step;
        time += frame_step;
    }

    static void blit_render_targets_to_screen(DisplayServerEnums::WindowID p_screen,
        const RenderingServerTypes::BlitToScreen* p_render_targets, int p_amount)
    {
    }

    _ALWAYS_INLINE_ static bool is_opengl() { return false; }
    _ALWAYS_INLINE_ static void gl_end_frame(bool p_swap_buffers) {}

    static void end_frame(bool p_present);
    static void finalize() {}

    static Error _create_current()
    {
        RasterizerDummy::initialize();
        return OK;
    }

    static void make_current()
    {
        RendererCompositor::_create_func = _create_current;
        RendererCompositor::low_end = false;
        RendererCompositor::bind_compositor<RasterizerDummy>();
    }

    _ALWAYS_INLINE_ static uint64_t get_frame_number() { return frame; }
    _ALWAYS_INLINE_ static double get_frame_delta_time() { return delta; }
    _ALWAYS_INLINE_ static double get_total_time() { return time; }
    _ALWAYS_INLINE_ static bool can_create_resources_async() { return false; }

    RasterizerDummy() = delete;
    RasterizerDummy(const RasterizerDummy&) = delete;
    ~RasterizerDummy() = delete;
};

using RDummy = RasterizerDummy;


