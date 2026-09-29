/**************************************************************************/
/*  rasterizer_gles3.h                                                    */
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

#ifdef GLES3_ENABLED

#include "drivers/gles3/effects/copy_effects.h"
#include "drivers/gles3/effects/cubemap_filter.h"
#include "drivers/gles3/effects/feed_effects.h"
#include "drivers/gles3/effects/glow.h"
#include "drivers/gles3/effects/post_effects.h"
#include "drivers/gles3/environment/fog.h"
#include "drivers/gles3/environment/gi.h"
#include "drivers/gles3/rasterizer_canvas_gles3.h"
#include "drivers/gles3/rasterizer_scene_gles3.h"
#include "drivers/gles3/storage/config.h"
#include "drivers/gles3/storage/light_storage.h"
#include "drivers/gles3/storage/material_storage.h"
#include "drivers/gles3/storage/mesh_storage.h"
#include "drivers/gles3/storage/particles_storage.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "drivers/gles3/storage/utilities.h"
#include "servers/rendering/renderer_compositor.h"

class RasterizerGLES3 final
{
private:
    static inline uint64_t frame = 1;
    static inline double delta = 0.0;
    static inline double time_total = 0.0;

#ifdef WINDOWS_ENABLED
    static inline bool screen_flipped_y = false;
#endif

    static inline GLES3::Config* config = nullptr;
    static inline GLES3::Utilities* utilities = nullptr;
    static inline GLES3::TextureStorage* texture_storage = nullptr;
    static inline GLES3::MaterialStorage* material_storage = nullptr;
    static inline GLES3::MeshStorage* mesh_storage = nullptr;
    static inline GLES3::ParticlesStorage* particles_storage = nullptr;
    static inline GLES3::LightStorage* light_storage = nullptr;
    static inline GLES3::GI* gi = nullptr;
    static inline GLES3::Fog* fog = nullptr;
    static inline GLES3::CopyEffects* copy_effects = nullptr;
    static inline GLES3::CubemapFilter* cubemap_filter = nullptr;
    static inline GLES3::Glow* glow = nullptr;
    static inline GLES3::PostEffects* post_effects = nullptr;
    static inline GLES3::FeedEffects* feed_effects = nullptr;
    static inline RasterizerCanvasGLES3* canvas = nullptr;
    static inline RasterizerSceneGLES3* scene = nullptr;

    static void _blit_render_target_to_screen(DisplayServerEnums::WindowID p_screen,
        const RenderingServerTypes::BlitToScreen& p_blit, bool p_first = true);

public:
    _ALWAYS_INLINE_ static RendererUtilities* get_utilities() { return utilities; }
    _ALWAYS_INLINE_ static RendererLightStorage* get_light_storage() { return light_storage; }
    _ALWAYS_INLINE_ static RendererMaterialStorage* get_material_storage() { return material_storage; }
    _ALWAYS_INLINE_ static RendererMeshStorage* get_mesh_storage() { return mesh_storage; }
    _ALWAYS_INLINE_ static RendererParticlesStorage* get_particles_storage() { return particles_storage; }
    _ALWAYS_INLINE_ static RendererTextureStorage* get_texture_storage() { return texture_storage; }
    _ALWAYS_INLINE_ static RendererGI* get_gi() { return gi; }
    _ALWAYS_INLINE_ static RendererFog* get_fog() { return fog; }
    _ALWAYS_INLINE_ static RendererCanvasRender* get_canvas() { return canvas; }
    _ALWAYS_INLINE_ static RendererSceneRender* get_scene() { return scene; }

    static void set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
        RSE::SplashStretchMode p_stretch_mode, bool p_use_filter = true);

    static void initialize();
    static void begin_frame(double frame_step);

    static void blit_render_targets_to_screen(DisplayServerEnums::WindowID p_screen,
        const RenderingServerTypes::BlitToScreen* p_render_targets, int p_amount);

    _ALWAYS_INLINE_ static bool is_opengl() { return true; }

    static void gl_end_frame(bool p_swap_buffers);
    static void end_frame(bool p_swap_buffers);

    static void finalize();

    static Error _create_current()
    {
        RasterizerGLES3::initialize();
        return OK;
    }

    static void make_current(bool p_gles_over_gl);

#ifdef WINDOWS_ENABLED
    _ALWAYS_INLINE_ static void set_screen_flipped_y(bool p_flipped) { screen_flipped_y = p_flipped; }
#endif

    _ALWAYS_INLINE_ static uint64_t get_frame_number() { return frame; }
    _ALWAYS_INLINE_ static double get_frame_delta_time() { return delta; }
    _ALWAYS_INLINE_ static double get_total_time() { return time_total; }
    _ALWAYS_INLINE_ static bool can_create_resources_async() { return false; }

    RasterizerGLES3() = delete;
    RasterizerGLES3(const RasterizerGLES3&) = delete;
    ~RasterizerGLES3() = delete;
};

using RGLES3 = RasterizerGLES3;

#endif // GLES3_ENABLED
