#!/usr/bin/env python3

"""
Standalone SCons-free code generator for Voltaire Engine.
Generates all required .gen.h / .gen.cpp / .gen.c files directly using the
repository's Python builder scripts (without invoking SCons or reading SConstruct).
"""

import os
import shutil
import subprocess
import sys

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT_DIR)
if ROOT_DIR not in sys.path:
    sys.path.insert(0, ROOT_DIR)
SCRIPTS_DIR = os.path.join(ROOT_DIR, "scripts")
if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)


WAYLAND_PROTOCOLS = [
    ("wayland", "thirdparty/wayland/protocol/wayland.xml"),
    ("tablet", "thirdparty/wayland-protocols/stable/tablet/tablet-v2.xml"),
    ("viewporter", "thirdparty/wayland-protocols/stable/viewporter/viewporter.xml"),
    ("xdg_shell", "thirdparty/wayland-protocols/stable/xdg-shell/xdg-shell.xml"),
    ("color_management", "thirdparty/wayland-protocols/staging/color-management/color-management-v1.xml"),
    ("cursor_shape", "thirdparty/wayland-protocols/staging/cursor-shape/cursor-shape-v1.xml"),
    ("fractional_scale", "thirdparty/wayland-protocols/staging/fractional-scale/fractional-scale-v1.xml"),
    ("xdg_activation", "thirdparty/wayland-protocols/staging/xdg-activation/xdg-activation-v1.xml"),
    ("xdg_system_bell", "thirdparty/wayland-protocols/staging/xdg-system-bell/xdg-system-bell-v1.xml"),
    ("xdg_toplevel_icon", "thirdparty/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml"),
    ("pointer_warp", "thirdparty/wayland-protocols/staging/pointer-warp/pointer-warp-v1.xml"),
    ("idle_inhibit", "thirdparty/wayland-protocols/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml"),
    ("pointer_constraints", "thirdparty/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml"),
    ("pointer_gestures", "thirdparty/wayland-protocols/unstable/pointer-gestures/pointer-gestures-unstable-v1.xml"),
    ("primary_selection", "thirdparty/wayland-protocols/unstable/primary-selection/primary-selection-unstable-v1.xml"),
    ("relative_pointer", "thirdparty/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml"),
    ("text_input", "thirdparty/wayland-protocols/unstable/text-input/text-input-unstable-v3.xml"),
    ("xdg_decoration", "thirdparty/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml"),
    ("xdg_foreign_v1", "thirdparty/wayland-protocols/unstable/xdg-foreign/xdg-foreign-unstable-v1.xml"),
    ("xdg_foreign_v2", "thirdparty/wayland-protocols/unstable/xdg-foreign/xdg-foreign-unstable-v2.xml"),
    ("linux_dmabuf_v1", "thirdparty/wayland-protocols/stable/linux-dmabuf/linux-dmabuf-v1.xml"),
    ("linux_explicit_synchronization_unstable_v1", "thirdparty/wayland-protocols/unstable/linux-explicit-synchronization/linux-explicit-synchronization-unstable-v1.xml"),
    ("fifo_v1", "thirdparty/wayland-protocols/staging/fifo/fifo-v1.xml"),
    ("commit_timing_v1", "thirdparty/wayland-protocols/staging/commit-timing/commit-timing-v1.xml"),
    ("linux_drm_syncobj_v1", "thirdparty/wayland-protocols/staging/linux-drm-syncobj/linux-drm-syncobj-v1.xml"),
    ("tearing_control_v1", "thirdparty/wayland-protocols/staging/tearing-control/tearing-control-v1.xml"),
    ("wayland-drm", "thirdparty/wayland-protocols/mesa/wayland-drm.xml"),
    ("godot_embedding_compositor", "platform/linuxbsd/wayland/godot-embedding-compositor.xml"),
]


def run_script(script_name, *args):
    cmd = [sys.executable, os.path.join("scripts", script_name), *map(str, args)]
    subprocess.run(cmd, check=True)


def generate_wayland_protocols(sowrap=True):
    scanner = shutil.which("wayland-scanner")
    proto_dir = "platform/linuxbsd/wayland/protocol"
    os.makedirs(proto_dir, exist_ok=True)
    if not scanner:
        return

    h_chunks = ["#pragma once\n"]
    c_chunks = ["/* THIS FILE IS GENERATED. EDITS WILL BE LOST. */\n"]
    wrap_inc = "wayland/dynwrappers/wayland-client-core-so_wrap.h"

    for name, xml_path in WAYLAND_PROTOCOLS:
        with open(xml_path, "rb") as xml_in:
            h_res = subprocess.run([scanner, "-c", "client-header"], stdin=xml_in, capture_output=True, check=True)
        h_txt = h_res.stdout.decode("utf-8")
        if sowrap:
            h_txt = h_txt.replace("wayland-client-core.h", wrap_inc)
        h_chunks.append(f"\n/* --- {name} --- */\n" + h_txt)

        with open(xml_path, "rb") as xml_in:
            c_res = subprocess.run([scanner, "-c", "private-code"], stdin=xml_in, capture_output=True, check=True)
        c_txt = c_res.stdout.decode("utf-8")
        if sowrap:
            c_txt = c_txt.replace("wayland-util.h", wrap_inc)
        c_chunks.append(f"\n/* --- {name} --- */\n" + c_txt)

    with open(os.path.join(proto_dir, "wayland_protocols.gen.h"), "w", encoding="utf-8", newline="\n") as f:
        f.write("".join(h_chunks))
    with open(os.path.join(proto_dir, "wayland_protocols.gen.c"), "w", encoding="utf-8", newline="\n") as f:
        f.write("".join(c_chunks))


def generate_all():
    single_scripts = [
        "version_info_builder.py",
        "version_hash_builder.py",
        "encryption_key_builder.py",
        "make_license_header.py",
        "make_default_controller_mappings.py",
        "make_splash.py",
        "make_default_theme_icons_action.py",
        "areatex_builder.py",
        "modules_enabled_builder.py",
        "register_module_types_builder.py",
        "register_platform_apis_builder.py",
        "register_exporters_builder.py",
        "export_icon_builder.py",
        "make_icu_data.py",
    ]
    for s in single_scripts:
        if os.path.exists(os.path.join("scripts", s)):
            run_script(s)

    for idx in range(4):
        run_script("make_translations.py", idx)
    run_script("make_fonts_header.py", 0)
    run_script("run.py", 0)

    run_script("build_raw_headers.py", "--all")
    run_script("build_rd_headers.py", "--all")
    run_script("build_gles3_headers.py", "--all")
    generate_wayland_protocols(sowrap=True)


if __name__ == "__main__":
    if "--if-needed" in sys.argv and os.path.exists("core/version_generated.gen.h") and os.path.exists("modules/modules_enabled.gen.h"):
        sys.exit(0)
    generate_all()
