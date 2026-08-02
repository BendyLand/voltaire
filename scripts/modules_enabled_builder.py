#!/usr/bin/env python3

import os
from collections import OrderedDict
from io import TextIOWrapper

import methods


def modules_enabled_builder(target, source):
    modules = sorted(source[0])
    with methods.generated_wrapper(str(target[0])) as file:
        for module in modules:
            file.write(f"#define MODULE_{module.upper()}_ENABLED\n")


modules_enabled_builder(
    ["modules/modules_enabled.gen.h"],
    [
        OrderedDict(
            [
                ("zip", "modules/zip"),
                ("xatlas_unwrap", "modules/xatlas_unwrap"),
                ("webxr", "modules/webxr"),
                ("websocket", "modules/websocket"),
                ("webrtc", "modules/webrtc"),
                ("webp", "modules/webp"),
                ("visual_shader", "modules/visual_shader"),
                ("vhacd", "modules/vhacd"),
                ("upnp", "modules/upnp"),
                ("tinyexr", "modules/tinyexr"),
                ("tga", "modules/tga"),
                ("regex", "modules/regex"),
                ("raycast", "modules/raycast"),
                ("openxr", "modules/openxr"),
                ("ogg", "modules/ogg"),
                ("objectdb_profiler", "modules/objectdb_profiler"),
                ("noise", "modules/noise"),
                ("navigation_3d", "modules/navigation_3d"),
                ("navigation_2d", "modules/navigation_2d"),
                ("multiplayer", "modules/multiplayer"),
                ("mp3", "modules/mp3"),
                ("mobile_vr", "modules/mobile_vr"),
                ("meshoptimizer", "modules/meshoptimizer"),
                ("mbedtls", "modules/mbedtls"),
                ("lightmapper_rd", "modules/lightmapper_rd"),
                ("jsonrpc", "modules/jsonrpc"),
                ("jpg", "modules/jpg"),
                ("jolt_physics", "modules/jolt_physics"),
                ("interactive_music", "modules/interactive_music"),
                ("hdr", "modules/hdr"),
                ("gridmap", "modules/gridmap"),
                ("godot_physics_3d", "modules/godot_physics_3d"),
                ("godot_physics_2d", "modules/godot_physics_2d"),
                ("glslang", "modules/glslang"),
                ("freetype", "modules/freetype"),
                ("etcpak", "modules/etcpak"),
                ("enet", "modules/enet"),
                ("dds", "modules/dds"),
                ("cvtt", "modules/cvtt"),
                ("csg", "modules/csg"),
                ("camera", "modules/camera"),
                ("bmp", "modules/bmp"),
                ("betsy", "modules/betsy"),
                ("bcdec", "modules/bcdec"),
                ("basis_universal", "modules/basis_universal"),
                ("astcenc", "modules/astcenc"),
                ("vorbis", "modules/vorbis"),
                ("theora", "modules/theora"),
                ("svg", "modules/svg"),
                ("msdfgen", "modules/msdfgen"),
                ("ktx", "modules/ktx"),
                ("gltf", "modules/gltf"),
                ("fbx", "modules/fbx"),
                ("text_server_adv", "modules/text_server_adv"),
            ]
        )
    ],
)
