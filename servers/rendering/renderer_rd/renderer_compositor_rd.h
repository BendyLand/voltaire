/**************************************************************************/
/*  renderer_compositor_rd.h                                              */
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

#include "core/io/image.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_rd/environment/fog.h"
#include "servers/rendering/renderer_rd/framebuffer_cache_rd.h"
#include "servers/rendering/renderer_rd/renderer_canvas_render_rd.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/shaders/blit.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/light_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/mesh_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/particles_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/utilities.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"

class RendererCompositorRD final
{
public:
    enum BlitMode
    {
        BLIT_MODE_NORMAL,
        BLIT_MODE_USE_LAYER,
        BLIT_MODE_LENS,
        BLIT_MODE_NORMAL_ALPHA,
        BLIT_MODE_MAX
    };

    struct BlitPushConstant
    {
        float src_rect[4];
        float dst_rect[4];

        float rotation_sin;
        float rotation_cos;
        float eye_center[2];

        float k1;
        float k2;
        float upscale;
        float aspect_ratio;

        uint32_t layer;
        uint32_t source_is_srgb;
        uint32_t use_debanding;
        uint32_t target_color_space;

        float reference_multiplier;
        float output_max_value;
        uint32_t pad[2];
    };

    struct BlitPipelines
    {
        RID pipelines[BLIT_MODE_MAX];
    };

    struct Blit
    {
        BlitPushConstant push_constant;
        BlitShaderRD shader;
        RID shader_version;
        HashMap<RenderingDevice::FramebufferFormatID, BlitPipelines> pipelines_by_format;
        RID index_buffer;
        RID array;
        RID sampler;
    };

private:
    static inline UniformSetCacheRD* uniform_set_cache = nullptr;
    static inline FramebufferCacheRD* framebuffer_cache = nullptr;

    static inline RendererCanvasRenderRD* canvas = nullptr;
    static inline RendererSceneRenderRD* scene = nullptr;

    static inline RendererRD::Fog* fog = nullptr;
    static inline RendererRD::LightStorage* light_storage = nullptr;
    static inline RendererRD::MaterialStorage* material_storage = nullptr;
    static inline RendererRD::MeshStorage* mesh_storage = nullptr;
    static inline RendererRD::ParticlesStorage* particles_storage = nullptr;
    static inline RendererRD::TextureStorage* texture_storage = nullptr;
    static inline RendererRD::Utilities* utilities = nullptr;

    static inline Blit* blit = nullptr;

    static inline HashMap<RID, RID> render_target_descriptors;

    static inline double time = 0.0;
    static inline double delta = 0.0;
    static inline uint64_t frame = 0;

    static BlitPipelines _get_blit_pipelines_for_format(RenderingDevice::FramebufferFormatID format);
    static float _compute_reference_multiplier(RDC::ColorSpace p_color_space,
        const float p_reference_luminance, const float p_linear_luminance_scale);

public:
    _ALWAYS_INLINE_ static RendererUtilities* get_utilities() { return utilities; }
    _ALWAYS_INLINE_ static RendererLightStorage* get_light_storage() { return light_storage; }
    _ALWAYS_INLINE_ static RendererMaterialStorage* get_material_storage() { return material_storage; }
    _ALWAYS_INLINE_ static RendererMeshStorage* get_mesh_storage() { return mesh_storage; }
    _ALWAYS_INLINE_ static RendererParticlesStorage* get_particles_storage() { return particles_storage; }
    _ALWAYS_INLINE_ static RendererTextureStorage* get_texture_storage() { return texture_storage; }

    static RendererGI* get_gi()
    {
        ERR_FAIL_NULL_V(scene, nullptr);
        return scene->get_gi();
    }

    _ALWAYS_INLINE_ static RendererFog* get_fog() { return fog; }
    _ALWAYS_INLINE_ static RendererCanvasRender* get_canvas() { return canvas; }
    _ALWAYS_INLINE_ static RendererSceneRenderRD* get_scene() { return scene; }

    static void set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
        RSE::SplashStretchMode p_stretch_mode, bool p_use_filter);

    static void initialize();
    static void begin_frame(double frame_step);
    static void blit_render_targets_to_screen(DisplayServerEnums::WindowID p_screen,
        const RenderingServerTypes::BlitToScreen* p_render_targets, int p_amount);

    _ALWAYS_INLINE_ static bool is_opengl() { return false; }
    _ALWAYS_INLINE_ static void gl_end_frame(bool p_swap_buffers) {}
    static void end_frame(bool p_present);
    static void finalize();

    _ALWAYS_INLINE_ static uint64_t get_frame_number() { return frame; }
    _ALWAYS_INLINE_ static double get_frame_delta_time() { return delta; }
    _ALWAYS_INLINE_ static double get_total_time() { return time; }
    _ALWAYS_INLINE_ static bool can_create_resources_async() { return true; }

    static bool is_xr_enabled() { return RendererCompositor::is_xr_enabled(); }
    _ALWAYS_INLINE_ static Error is_viable() { return OK; }

    static Error _create_current()
    {
        RendererCompositorRD::initialize();
        return OK;
    }

    static void make_current()
    {
        RendererCompositor::_create_func = _create_current;
        RendererCompositor::low_end = false;
        RendererCompositor::bind_compositor<RendererCompositorRD>();
    }

    RendererCompositorRD() = delete;
    RendererCompositorRD(const RendererCompositorRD&) = delete;
    ~RendererCompositorRD() = delete;
};

using RCRD = RendererCompositorRD;
