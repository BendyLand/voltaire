#!/usr/bin/env python3

from collections import OrderedDict

import methods


def register_module_types_builder(target, source):
    modules = source[0]
    mod_inc = "\n".join(
        [f'#include "{value}/register_types.h"' for value in modules.values()]
    )
    mod_init = "\n".join([f"""\
#ifdef MODULE_{key.upper()}_ENABLED
	initialize_{key}_module(p_level);
#endif""" for key in modules.keys()])
    mod_uninit = "\n".join([f"""\
#ifdef MODULE_{key.upper()}_ENABLED
	uninitialize_{key}_module(p_level);
#endif""" for key in modules.keys()])
    with methods.generated_wrapper(str(target[0])) as file:
        file.write(f"""\
#include "register_module_types.h"

#include "modules/modules_enabled.gen.h"

// IWYU pragma: begin_keep.
{mod_inc}
// IWYU pragma: end_keep.

void initialize_modules(ModuleInitializationLevel p_level) {{
{mod_init}
}}

void uninitialize_modules(ModuleInitializationLevel p_level) {{
{mod_uninit}
}}
""")


register_module_types_builder(
    ["modules/register_module_types.gen.cpp"],
    [
        OrderedDict(
            [
                ("astcenc", "modules/astcenc"),
                ("basis_universal", "modules/basis_universal"),
                ("bcdec", "modules/bcdec"),
                ("betsy", "modules/betsy"),
                ("bmp", "modules/bmp"),
                ("camera", "modules/camera"),
                ("csg", "modules/csg"),
                ("cvtt", "modules/cvtt"),
                ("dds", "modules/dds"),
                ("enet", "modules/enet"),
                ("etcpak", "modules/etcpak"),
                ("fbx", "modules/fbx"),
                ("freetype", "modules/freetype"),
                ("glslang", "modules/glslang"),
                ("gltf", "modules/gltf"),
                ("godot_physics_2d", "modules/godot_physics_2d"),
                ("godot_physics_3d", "modules/godot_physics_3d"),
                ("gridmap", "modules/gridmap"),
                ("hdr", "modules/hdr"),
                ("interactive_music", "modules/interactive_music"),
                ("jolt_physics", "modules/jolt_physics"),
                ("jpg", "modules/jpg"),
                ("jsonrpc", "modules/jsonrpc"),
                ("ktx", "modules/ktx"),
                ("lightmapper_rd", "modules/lightmapper_rd"),
                ("mbedtls", "modules/mbedtls"),
                ("meshoptimizer", "modules/meshoptimizer"),
                ("mobile_vr", "modules/mobile_vr"),
                ("mono", "modules/mono"),
                ("mp3", "modules/mp3"),
                ("msdfgen", "modules/msdfgen"),
                ("multiplayer", "modules/multiplayer"),
                ("navigation_2d", "modules/navigation_2d"),
                ("navigation_3d", "modules/navigation_3d"),
                ("noise", "modules/noise"),
                ("objectdb_profiler", "modules/objectdb_profiler"),
                ("ogg", "modules/ogg"),
                ("openxr", "modules/openxr"),
                ("raycast", "modules/raycast"),
                ("regex", "modules/regex"),
                ("svg", "modules/svg"),
                ("text_server_adv", "modules/text_server_adv"),
                ("text_server_fb", "modules/text_server_fb"),
                ("tga", "modules/tga"),
                ("theora", "modules/theora"),
                ("tinyexr", "modules/tinyexr"),
                ("upnp", "modules/upnp"),
                ("vhacd", "modules/vhacd"),
                ("visual_shader", "modules/visual_shader"),
                ("vorbis", "modules/vorbis"),
                ("webp", "modules/webp"),
                ("webrtc", "modules/webrtc"),
                ("websocket", "modules/websocket"),
                ("webxr", "modules/webxr"),
                ("xatlas_unwrap", "modules/xatlas_unwrap"),
                ("zip", "modules/zip"),
            ]
        ),
        "modules/modules_enabled.gen.h",
    ],
)
