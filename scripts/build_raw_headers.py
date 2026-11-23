#!/usr/bin/env python3

"""Standalone raw GLSL header generator synced with root glsl_builders.py (no SCons required)."""

import os
import sys

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if ROOT_DIR not in sys.path:
    sys.path.insert(0, ROOT_DIR)

import glsl_builders


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

BUNDLE_TARGET = "modules/lightmapper_rd/rd_raw_shaders.gen.h"


if __name__ == "__main__":
    glsl_builders.build_raw_headers_bundle(BUNDLE_TARGET, RAW_SHADERS)
