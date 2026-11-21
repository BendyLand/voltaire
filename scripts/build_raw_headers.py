#!/usr/bin/env python3

"""Standalone raw GLSL header generator synced with root glsl_builders.py (no SCons required)."""

import os
import sys

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if ROOT_DIR not in sys.path:
    sys.path.insert(0, ROOT_DIR)

import glsl_builders
import utils


def build_raw_headers(target, source):
    for src in source:
        glsl_builders.build_raw_header(f"{src}.gen.h", str(src))


RAW_SHADERS = [
    "modules/betsy/alpha_stitch.glsl",
    "modules/betsy/bc1.glsl",
    "modules/betsy/bc4.glsl",
    "modules/betsy/bc6h.glsl",
    "modules/betsy/rgb_to_rgba.glsl",
    "modules/lightmapper_rd/lm_blendseams.glsl",
    "modules/lightmapper_rd/lm_compute.glsl",
    "modules/lightmapper_rd/lm_raster.glsl",
    "servers/rendering/renderer_rd/shaders/decal_data_inc.glsl",
    "servers/rendering/renderer_rd/shaders/light_data_inc.glsl",
    "servers/rendering/renderer_rd/shaders/scene_data_inc.glsl",
    "servers/rendering/renderer_rd/shaders/area_lights_inc.glsl",
    "servers/rendering/renderer_rd/shaders/canvas_uniforms_inc.glsl",
    "servers/rendering/renderer_rd/shaders/cluster_data_inc.glsl",
    "servers/rendering/renderer_rd/shaders/half_inc.glsl",
    "servers/rendering/renderer_rd/shaders/oct_inc.glsl",
    "servers/rendering/renderer_rd/shaders/samplers_inc.glsl",
    "servers/rendering/renderer_rd/shaders/scene_forward_aa_inc.glsl",
    "servers/rendering/renderer_rd/shaders/scene_forward_gi_inc.glsl",
    "servers/rendering/renderer_rd/shaders/scene_forward_lights_inc.glsl",
    "servers/rendering/renderer_rd/shaders/scene_forward_vertex_lights_inc.glsl",
]


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--all":
        for s in RAW_SHADERS:
            glsl_builders.build_raw_header(f"{s}.gen.h", s)
    else:
        run = utils.get_run_arg()
        if 0 <= run < len(RAW_SHADERS):
            s = RAW_SHADERS[run]
            build_raw_headers([f"{s}.gen.h"], [s])
        else:
            print("No commands left to run.")
