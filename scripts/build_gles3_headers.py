#!/usr/bin/env python3

"""Standalone GLES3 shader header generator synced with root gles3_builders.py (no SCons required)."""

import os
import sys

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if ROOT_DIR not in sys.path:
    sys.path.insert(0, ROOT_DIR)

import gles3_builders
import utils


def build_gles3_headers(target, source):
    for src in source:
        gles3_builders.build_gles3_header(f"{src}.gen.h", str(src))


GLES3_SHADERS = [
    "drivers/gles3/shaders/effects/copy.glsl",
    "drivers/gles3/shaders/effects/cubemap_filter.glsl",
    "drivers/gles3/shaders/feed.glsl",
    "drivers/gles3/shaders/effects/glow.glsl",
    "drivers/gles3/shaders/effects/post.glsl",
    "drivers/gles3/shaders/canvas.glsl",
    "drivers/gles3/shaders/canvas_occlusion.glsl",
    "drivers/gles3/shaders/particles.glsl",
    "drivers/gles3/shaders/scene.glsl",
    "drivers/gles3/shaders/sky.glsl",
    "drivers/gles3/shaders/tex_blit.glsl",
    "drivers/gles3/shaders/skeleton.glsl",
    "drivers/gles3/shaders/particles_copy.glsl",
    "drivers/gles3/shaders/canvas_sdf.glsl",
]


BUNDLE_TARGET = "drivers/gles3/shaders/gles3_shaders.gen.h"

if __name__ == "__main__":
    gles3_builders.build_gles3_headers_bundle(BUNDLE_TARGET, GLES3_SHADERS)
