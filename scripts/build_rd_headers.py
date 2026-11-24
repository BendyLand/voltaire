#!/usr/bin/env python3

"""Standalone RD shader header generator synced with root glsl_builders.py (no SCons required)."""

import os
import sys

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if ROOT_DIR not in sys.path:
    sys.path.insert(0, ROOT_DIR)

import glsl_builders
import utils


def build_rd_headers(target, source):
    for src in source:
        glsl_builders.build_rd_header(f"{src}.gen.h", str(src))


SHADERS = [
    "servers/rendering/renderer_rd/shaders/effects/bokeh_dof.glsl",
    "servers/rendering/renderer_rd/shaders/effects/bokeh_dof_raster.glsl",
    "servers/rendering/renderer_rd/shaders/effects/blur_raster.glsl",
    "servers/rendering/renderer_rd/shaders/effects/copy.glsl",
    "servers/rendering/renderer_rd/shaders/effects/copy_to_fb.glsl",
    "servers/rendering/renderer_rd/shaders/effects/cube_to_dp.glsl",
    "servers/rendering/renderer_rd/shaders/effects/cube_to_octmap.glsl",
    "servers/rendering/renderer_rd/shaders/effects/octmap_downsampler.glsl",
    "servers/rendering/renderer_rd/shaders/effects/octmap_downsampler_raster.glsl",
    "servers/rendering/renderer_rd/shaders/effects/octmap_filter.glsl",
    "servers/rendering/renderer_rd/shaders/effects/octmap_filter_raster.glsl",
    "servers/rendering/renderer_rd/shaders/effects/octmap_roughness.glsl",
    "servers/rendering/renderer_rd/shaders/effects/octmap_roughness_raster.glsl",
    "servers/rendering/renderer_rd/shaders/effects/specular_merge.glsl",
    "servers/rendering/renderer_rd/shaders/effects/motion_vectors.glsl",
    "servers/rendering/renderer_rd/shaders/effects/shadow_frustum.glsl",
    "servers/rendering/renderer_rd/shaders/effects/fsr_upscale.glsl",
    "servers/rendering/renderer_rd/shaders/effects/luminance_reduce.glsl",
    "servers/rendering/renderer_rd/shaders/effects/luminance_reduce_raster.glsl",
    "servers/rendering/renderer_rd/shaders/effects/resolve.glsl",
    "servers/rendering/renderer_rd/shaders/effects/resolve_raster.glsl",
    "servers/rendering/renderer_rd/shaders/effects/smaa_blending.glsl",
    "servers/rendering/renderer_rd/shaders/effects/smaa_edge_detection.glsl",
    "servers/rendering/renderer_rd/shaders/effects/smaa_weight_calculation.glsl",
    "servers/rendering/renderer_rd/shaders/effects/tonemap.glsl",
    "servers/rendering/renderer_rd/shaders/effects/tonemap_mobile.glsl",
    "servers/rendering/renderer_rd/shaders/effects/vrs.glsl",
    "servers/rendering/renderer_rd/shaders/environment/gi.glsl",
    "servers/rendering/renderer_rd/shaders/environment/sdfgi_debug.glsl",
    "servers/rendering/renderer_rd/shaders/environment/sdfgi_debug_probes.glsl",
    "servers/rendering/renderer_rd/shaders/environment/sdfgi_direct_light.glsl",
    "servers/rendering/renderer_rd/shaders/environment/sdfgi_integrate.glsl",
    "servers/rendering/renderer_rd/shaders/environment/sdfgi_preprocess.glsl",
    "servers/rendering/renderer_rd/shaders/environment/voxel_gi.glsl",
    "servers/rendering/renderer_rd/shaders/environment/voxel_gi_debug.glsl",
    "servers/rendering/renderer_rd/shaders/environment/sky.glsl",
    "servers/rendering/renderer_rd/shaders/canvas_sdf.glsl",
    "servers/rendering/renderer_rd/shaders/tex_blit.glsl",
    "servers/rendering/renderer_rd/shaders/cluster_debug.glsl",
    "servers/rendering/renderer_rd/shaders/cluster_render.glsl",
    "servers/rendering/renderer_rd/shaders/cluster_store.glsl",
    "servers/rendering/renderer_rd/shaders/blit.glsl",
    "servers/rendering/renderer_rd/shaders/environment/volumetric_fog.glsl",
    "servers/rendering/renderer_rd/shaders/environment/volumetric_fog_process.glsl",
    "servers/rendering/renderer_rd/shaders/canvas.glsl",
    "servers/rendering/renderer_rd/shaders/canvas_occlusion.glsl",
    "servers/rendering/renderer_rd/shaders/skeleton.glsl",
    "servers/rendering/renderer_rd/shaders/particles.glsl",
    "servers/rendering/renderer_rd/shaders/particles_copy.glsl",
    "servers/rendering/renderer_rd/shaders/forward_clustered/best_fit_normal.glsl",
    "servers/rendering/renderer_rd/shaders/forward_clustered/integrate_dfg.glsl",
    "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_accumulate_pass.glsl",
    "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_autogen_reactive_pass.glsl",
    "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_compute_luminance_pyramid_pass.glsl",
    "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_depth_clip_pass.glsl",
    "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_lock_pass.glsl",
    "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_rcas_pass.glsl",
    "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_reconstruct_previous_depth_pass.glsl",
    "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_tcr_autogen_pass.glsl",
    "servers/rendering/renderer_rd/shaders/effects/motion_vectors_store.glsl",
    "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection.glsl",
    "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_downsample.glsl",
    "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_filter.glsl",
    "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_hiz.glsl",
    "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_resolve.glsl",
    "servers/rendering/renderer_rd/shaders/effects/ss_effects_downsample.glsl",
    "servers/rendering/renderer_rd/shaders/effects/ssao.glsl",
    "servers/rendering/renderer_rd/shaders/effects/ssao_blur.glsl",
    "servers/rendering/renderer_rd/shaders/effects/ssao_importance_map.glsl",
    "servers/rendering/renderer_rd/shaders/effects/ssao_interleave.glsl",
    "servers/rendering/renderer_rd/shaders/effects/ssil.glsl",
    "servers/rendering/renderer_rd/shaders/effects/ssil_blur.glsl",
    "servers/rendering/renderer_rd/shaders/effects/ssil_importance_map.glsl",
    "servers/rendering/renderer_rd/shaders/effects/ssil_interleave.glsl",
    "servers/rendering/renderer_rd/shaders/effects/subsurface_scattering.glsl",
    "servers/rendering/renderer_rd/shaders/effects/taa_resolve.glsl",
    "servers/rendering/renderer_rd/shaders/forward_clustered/scene_forward_clustered.glsl",
    "servers/rendering/renderer_rd/shaders/forward_mobile/scene_forward_mobile.glsl",
    "servers/rendering/renderer_rd/shaders/effects/roughness_limiter.glsl",
    "servers/rendering/renderer_rd/shaders/effects/sort.glsl",
    "servers/rendering/renderer_rd/shaders/environment/voxel_gi_sdf.glsl",
    "servers/rendering/renderer_rd/shaders/giprobe_write.glsl",
]


RAW_SHADERS = [
    "modules/betsy/alpha_stitch.glsl",
    "modules/betsy/bc1.glsl",
    "modules/betsy/bc4.glsl",
    "modules/betsy/bc6h.glsl",
    "modules/betsy/rgb_to_rgba.glsl",
    "modules/lightmapper_rd/lm_blendseams.glsl",
    "modules/lightmapper_rd/lm_compute.glsl",
    "modules/lightmapper_rd/lm_raster.glsl",
]

BUNDLE_TARGET = "servers/rendering/renderer_rd/shaders/rd_shaders.gen.h"

if __name__ == "__main__":
    glsl_builders.build_rd_headers_bundle(BUNDLE_TARGET, SHADERS, RAW_SHADERS)
