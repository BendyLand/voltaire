#!/usr/bin/env python3
from __future__ import annotations

"""
Single unified SCons-free code generator & build utility module for Voltaire Engine.
Consolidates header/shader/translation/Wayland/ICU/splash/license/version generation and helper utilities.
"""

import os
import os.path
import platform
import shutil
import subprocess
import sys
import tempfile
import uuid
from collections import OrderedDict
from io import TextIOWrapper
from pathlib import Path

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if __name__ == "__main__":
    os.chdir(ROOT_DIR)
if ROOT_DIR not in sys.path:
    sys.path.insert(0, ROOT_DIR)

methods = sys.modules[__name__]

import atexit
import contextlib
import glob
import math
import os
import re
import shlex
import subprocess
import sys
import zlib
from collections import OrderedDict
from io import StringIO
from pathlib import Path
from typing import Generator, TextIO

from misc.utility.color import print_error, print_info, print_warning

base_folder = Path(ROOT_DIR)

compiler_version_cache = None


def get_version_info(module_version_string="", silent=False):
    build_name = "custom_build"
    if os.getenv("BUILD_NAME") is not None:
        build_name = str(os.getenv("BUILD_NAME"))
        if not silent:
            print_info(f"Using custom build name: '{build_name}'.")

    import version

    version_info = {
        "short_name": str(version.short_name),
        "name": str(version.name),
        "major": int(version.major),
        "minor": int(version.minor),
        "patch": int(version.patch),
        "status": str(version.status),
        "build": str(build_name),
        "module_config": str(version.module_config) + module_version_string,
        "website": str(version.website),
        "docs_branch": str(version.docs),
    }

    if os.getenv("VLTR_VERSION_STATUS") is not None:
        version_info["status"] = str(os.getenv("VLTR_VERSION_STATUS"))
        if not silent:
            print_info(f"Using version status '{version_info['status']}', overriding the original '{version.status}'.")

    return version_info


def get_git_info():
    os.chdir(base_folder)

    git_hash = ""
    git_folder = ".git"

    if os.path.isfile(".git"):
        with open(".git", "r", encoding="utf-8") as file:
            module_folder = file.readline().strip()
        if module_folder.startswith("gitdir: "):
            git_folder = module_folder[8:]

    if os.path.isfile(os.path.join(git_folder, "HEAD")):
        with open(os.path.join(git_folder, "HEAD"), "r", encoding="utf8") as file:
            head = file.readline().strip()
        if head.startswith("ref: "):
            ref = head[5:]
            parts = git_folder.split("/")
            if len(parts) > 2 and parts[-2] == "worktrees":
                git_folder = "/".join(parts[0:-2])
            head = os.path.join(git_folder, ref)
            packedrefs = os.path.join(git_folder, "packed-refs")
            if os.path.isfile(head):
                with open(head, "r", encoding="utf-8") as file:
                    git_hash = file.readline().strip()
            elif os.path.isfile(packedrefs):
                for line in open(packedrefs, "r", encoding="utf-8").read().splitlines():
                    if line.startswith("#"):
                        continue
                    (line_hash, line_ref) = line.split(" ")
                    if ref == line_ref:
                        git_hash = line_hash
                        break
        else:
            git_hash = head

    git_timestamp = 0
    if os.path.exists(".git"):
        try:
            git_timestamp = subprocess.check_output(
                ["git", "log", "-1", "--pretty=format:%ct", "--no-show-signature", git_hash], encoding="utf-8"
            )
        except (subprocess.CalledProcessError, OSError):
            pass

    return {
        "git_hash": git_hash,
        "git_timestamp": git_timestamp,
    }


def detect_modules(search_path, recursive=False):
    """Detects and collects a list of C++ modules at specified path."""
    modules = OrderedDict()

    def add_module(path):
        module_name = os.path.basename(path)
        module_path = path.replace("\\", "/")
        modules[module_name] = module_path

    def is_engine(path):
        version_path = os.path.join(path, "version.py")
        if os.path.exists(version_path):
            with open(version_path, "r", encoding="utf-8") as f:
                if 'short_name = "godot"' in f.read() or 'short_name = "voltaire"' in f.read():
                    return True
        return False

    def get_files(path):
        files = glob.glob(os.path.join(path, "*"))
        files.sort()
        return files

    if not recursive and os.path.basename(os.path.normpath(search_path)) == "modules":
        for mname in ['astcenc', 'basis_universal', 'bcdec', 'betsy', 'bmp', 'camera', 'csg', 'cvtt', 'dds', 'enet', 'etcpak', 'fbx', 'freetype', 'glslang', 'gltf', 'godot_physics_2d', 'gridmap', 'hdr', 'interactive_music', 'jolt_physics', 'jpg', 'ktx', 'lightmapper_rd', 'mbedtls', 'meshoptimizer', 'mobile_vr', 'mp3', 'msdfgen', 'multiplayer', 'navigation_2d', 'navigation_3d', 'noise', 'ogg', 'openxr', 'raycast', 'regex', 'svg', 'text_server_adv', 'tga', 'theora', 'tinyexr', 'upnp', 'vhacd', 'vorbis', 'webp', 'webrtc', 'websocket', 'webxr', 'xatlas_unwrap', 'zip']:
            modules[mname] = search_path.replace("\\", "/") + "/" + mname
        for path in get_files(search_path):
            if is_engine(path):
                continue
            if is_module(path) and os.path.basename(path) not in modules:
                add_module(path)
    elif not recursive:
        if is_module(search_path):
            add_module(search_path)
        for path in get_files(search_path):
            if is_engine(path):
                continue
            if is_module(path):
                add_module(path)
    else:
        to_search = [search_path]
        while to_search:
            path = to_search.pop()
            if is_module(path):
                add_module(path)
            for child in get_files(path):
                if not os.path.isdir(child):
                    continue
                if is_engine(child):
                    continue
                to_search.insert(0, child)
    return modules


def is_module(path):
    if not os.path.isdir(path):
        return False
    return os.path.exists(os.path.join(path, "register_types.h"))


def convert_custom_modules_path(path):
    if not path:
        return path
    path = os.path.realpath(os.path.expanduser(os.path.expandvars(path)))
    err_msg = "Build option 'custom_modules' must %s"
    if not os.path.isdir(path):
        raise ValueError(err_msg % "point to an existing directory.")
    if path == os.path.realpath("modules"):
        raise ValueError(err_msg % "be a directory other than built-in `modules` directory.")
    return path


def detect_darwin_toolchain_path(env):
    var_name = "APPLE_TOOLCHAIN_PATH"
    if not env.get(var_name):
        try:
            xcode_path = subprocess.check_output(["xcode-select", "-p"]).strip().decode("utf-8")
            if xcode_path:
                env[var_name] = xcode_path + "/Toolchains/XcodeDefault.xctoolchain"
        except (subprocess.CalledProcessError, OSError):
            print_error("Failed to find SDK path while running 'xcode-select -p'.")
            raise


def detect_darwin_sdk_path(platform, env):
    sdk_map = {
        "macos": ("macosx", "MACOS_SDK_PATH"),
        "ios": ("iphoneos", "APPLE_SDK_PATH"),
        "iossimulator": ("iphonesimulator", "APPLE_SDK_PATH"),
        "visionos": ("xros", "APPLE_SDK_PATH"),
        "visionossimulator": ("xrsimulator", "APPLE_SDK_PATH"),
    }
    if platform not in sdk_map:
        raise ValueError(f"Invalid platform argument passed to detect_darwin_sdk_path: {platform}")
    sdk_name, var_name = sdk_map[platform]

    if not env.get(var_name):
        try:
            sdk_path = subprocess.check_output(["xcrun", "--sdk", sdk_name, "--show-sdk-path"]).strip().decode("utf-8")
            if sdk_path:
                env[var_name] = sdk_path
        except (subprocess.CalledProcessError, OSError):
            print_error(f"Failed to find SDK path while running 'xcrun --sdk {sdk_name} --show-sdk-path'.")
            raise


def is_apple_clang(env):
    if env.get("platform") not in ["macos", "ios", "visionos"]:
        return False
    if not using_clang(env):
        return False
    try:
        cxx = env["CXX"]
        version = subprocess.check_output(shlex.split(cxx, posix=False) + ["--version"]).strip().decode("utf-8")
    except (subprocess.CalledProcessError, OSError):
        print_warning("Couldn't parse CXX environment variable to infer compiler version.")
        return False
    return version.startswith("Apple")


def get_compiler_version(env):
    global compiler_version_cache
    if compiler_version_cache is not None:
        return compiler_version_cache

    ret = {
        "major": -1,
        "minor": -1,
        "patch": -1,
        "metadata1": "",
        "metadata2": "",
        "date": "",
        "apple_major": -1,
        "apple_minor": -1,
        "apple_patch1": -1,
        "apple_patch2": -1,
        "apple_patch3": -1,
    }

    try:
        cxx = env.get("CXX", "c++")
        version = subprocess.check_output(
            shlex.split(cxx, posix=False) + ["--version"], shell=(os.name == "nt"), encoding="utf-8"
        ).strip()
    except (subprocess.CalledProcessError, OSError):
        return update_compiler_version_cache(ret)

    match = re.search(
        r"(?:(?<=version )|(?<=\) )|(?<=^))"
        r"(?P<major>\d+)"
        r"(?:\.(?P<minor>\d*))?"
        r"(?:\.(?P<patch>\d*))?"
        r"(?:-(?P<metadata1>[0-9a-zA-Z-]*))?"
        r"(?:\+(?P<metadata2>[0-9a-zA-Z-]*))?"
        r"(?: (?P<date>[0-9]{8}|[0-9]{6})(?![0-9a-zA-Z]))?",
        version,
    )
    if match is not None:
        for key, value in match.groupdict().items():
            if value is not None:
                ret[key] = value

    match_apple = re.search(
        r"(?:(?<=clang-)|(?<=\) )|(?<=^))"
        r"(?P<apple_major>\d+)"
        r"(?:\.(?P<apple_minor>\d*))?"
        r"(?:\.(?P<apple_patch1>\d*))?"
        r"(?:\.(?P<apple_patch2>\d*))?"
        r"(?:\.(?P<apple_patch3>\d*))?",
        version,
    )
    if match_apple is not None:
        for key, value in match_apple.groupdict().items():
            if value is not None:
                ret[key] = value

    for key in [
        "major",
        "minor",
        "patch",
        "apple_major",
        "apple_minor",
        "apple_patch1",
        "apple_patch2",
        "apple_patch3",
    ]:
        ret[key] = int(ret[key] or -1)
    return update_compiler_version_cache(ret)


def update_compiler_version_cache(value):
    global compiler_version_cache
    compiler_version_cache = value
    return value


def using_gcc(env):
    return "gcc" in os.path.basename(env.get("CC", ""))


def using_clang(env):
    return "clang" in os.path.basename(env.get("CC", ""))


def using_emcc(env):
    return "emcc" in os.path.basename(env.get("CC", ""))


def convert_size(size_bytes: int) -> str:
    if size_bytes == 0:
        return "0 bytes"
    SIZE_NAMES = ["bytes", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"]
    index = math.floor(math.log(size_bytes, 1024))
    power = math.pow(1024, index)
    size = round(size_bytes / power, 2)
    return f"{size} {SIZE_NAMES[index]}"


def get_size(start_path: str = ".") -> int:
    total_size = 0
    for dirpath, _, filenames in os.walk(start_path):
        for file in filenames:
            path = os.path.join(dirpath, file)
            total_size += os.path.getsize(path)
    return total_size


def prepare_timer():
    import time

    def print_elapsed_time(time_at_start: float):
        time_elapsed = time.time() - time_at_start
        time_formatted = time.strftime("%H:%M:%S", time.gmtime(time_elapsed))
        time_centiseconds = (time_elapsed % 1) * 100
        print_info(f"Time elapsed: {time_formatted}.{time_centiseconds:02.0f}")

    atexit.register(print_elapsed_time, time.time())


def generate_cpp_hint_file(filename):
    if os.path.isfile(filename):
        return
    try:
        with open(filename, "w", encoding="utf-8", newline="\n") as fd:
            fd.write("#define VLTRCLASS(m_class, m_inherits)\n")
            for name in ["VLTRVIRTUAL", "EXBIND", "MODBIND"]:
                for count in range(13):
                    for suffix in ["", "R", "C", "RC"]:
                        fd.write(f"#define {name}{count}{suffix}(")
                        if "R" in suffix:
                            fd.write("m_ret, ")
                        fd.write("m_name")
                        for idx in range(1, count + 1):
                            fd.write(f", type{idx}")
                        fd.write(")\n")
    except OSError:
        print_warning("Could not write cpp.hint file.")


############################################################
# FILE GENERATION & FORMATTING
############################################################


def generate_copyright_header(filename: str) -> str:
    MARGIN = 70
    TEMPLATE = """\
/**************************************************************************/
/*  %s*/
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                        https://blandlogic.com                          */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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
"""
    if len(filename := os.path.basename(filename).ljust(MARGIN)) > MARGIN:
        print_warning(f'Filename "{filename}" too large for copyright header.')
    return TEMPLATE % filename


@contextlib.contextmanager
def generated_wrapper(
    path: str,
    guard: bool | None = None,
) -> Generator[TextIO, None, None]:
    """
    Wrapper class to automatically handle copyright headers and header guards
    for generated scripts. Meant to be invoked via `with` statement similar to
    creating a file.
    """

    with open(path, "wt", encoding="utf-8", newline="\n") as file:
        if not path.endswith(".out"):
            file.write(generate_copyright_header(path))
            file.write("\n/* THIS FILE IS GENERATED. EDITS WILL BE LOST. */\n\n")

            if guard is None:
                guard = path.endswith((".h", ".hh", ".hpp", ".hxx", ".inc"))
            if guard:
                file.write("#pragma once\n\n")

        with StringIO(newline="\n") as str_io:
            yield str_io
            file.write(str_io.getvalue().strip() or "/* NO CONTENT */")

        file.write("\n")


def get_buffer(path: str) -> bytes:
    with open(path, "rb") as file:
        return file.read()


def compress_buffer(buffer: bytes) -> bytes:
    return zlib.compress(buffer, zlib.Z_BEST_COMPRESSION)


def format_buffer(buffer: bytes, indent: int = 0, width: int = 120) -> str:
    return re.sub(f"(.{{0,{width - indent - 1}}},) ", ("\t" * indent) + "\\g<1>\n", ", ".join(map(str, buffer)))


############################################################
# CSTRING PARSING
############################################################

C_ESCAPABLES = [
    ("\\", "\\\\"),
    ("\a", "\\a"),
    ("\b", "\\b"),
    ("\f", "\\f"),
    ("\n", "\\n"),
    ("\r", "\\r"),
    ("\t", "\\t"),
    ("\v", "\\v"),
    ('"', '\\"'),
]
C_ESCAPE_TABLE = str.maketrans(dict((x, y) for x, y in C_ESCAPABLES))


def to_escaped_cstring(value: str) -> str:
    return value.translate(C_ESCAPE_TABLE)


def to_raw_cstring(value: str | list[str]) -> str:
    MAX_LITERAL = 16 * 1024

    if isinstance(value, list):
        value = "\n".join(value) + "\n"

    split: list[bytes] = []
    offset = 0
    encoded = value.encode()

    while offset <= len(encoded):
        segment = encoded[offset : offset + MAX_LITERAL]
        offset += MAX_LITERAL
        if len(segment) == MAX_LITERAL:
            pretty_break = segment.rfind(b"\n\n")
            if pretty_break != -1:
                segment = segment[: pretty_break + 1]
                offset -= MAX_LITERAL - pretty_break - 1
            elif segment[-1] & 0b10000000:
                last_11xxxxxx_index = [i for i in range(-1, -5, -1) if segment[i] & 0b11000000 == 0b11000000][0]
                last_11xxxxxx = segment[last_11xxxxxx_index]
                if not last_11xxxxxx & 0b00100000:
                    last_char_length = 2
                elif not last_11xxxxxx & 0b0010000:
                    last_char_length = 3
                elif not last_11xxxxxx & 0b0001000:
                    last_char_length = 4

                if last_char_length > -last_11xxxxxx_index:
                    segment = segment[:last_11xxxxxx_index]
                    offset += last_11xxxxxx_index

        split += [segment]

    if len(split) == 1:
        return f'R"<!>({split[0].decode()})<!>"'
    else:
        return "({})".format(" ".join(f'R"<!>({segment.decode()})<!>"' for segment in split))

import os
import platform
import shutil
import subprocess
import sys


compatibility_platform_aliases = {
    "osx": "macos",
    "iphone": "ios",
    "x11": "linuxbsd",
    "javascript": "web",
}

# CPU architecture options.
architectures = ["x86_32", "x86_64", "arm32", "arm64", "rv64", "ppc64", "wasm32", "wasm64", "loongarch64"]
architecture_aliases = {
    "x86": "x86_32",
    "x64": "x86_64",
    "amd64": "x86_64",
    "armv7": "arm32",
    "armv8": "arm64",
    "arm64v8": "arm64",
    "aarch64": "arm64",
    "rv": "rv64",
    "riscv": "rv64",
    "riscv64": "rv64",
    "ppc64le": "ppc64",
    "loong64": "loongarch64",
}


def detect_arch():
    host_machine = platform.machine().lower()
    if host_machine in architectures:
        return host_machine
    elif host_machine in architecture_aliases.keys():
        return architecture_aliases[host_machine]
    elif "86" in host_machine:
        return "x86_32"
    else:
        methods.print_warning(f'Unsupported CPU architecture: "{host_machine}". Falling back to x86_64.')
        return "x86_64"


def validate_arch(arch, platform_name, supported_arches):
    if arch not in supported_arches:
        methods.print_error(
            'Unsupported CPU architecture "%s" for %s. Supported architectures are: %s.'
            % (arch, platform_name, ", ".join(supported_arches))
        )
        sys.exit(255)


def get_build_version(short):
    import version

    name = "custom_build"
    if os.getenv("BUILD_NAME") is not None:
        name = os.getenv("BUILD_NAME")
    v = "%d.%d" % (version.major, version.minor)
    if version.patch > 0:
        v += ".%d" % version.patch
    status = version.status
    if not short:
        if os.getenv("GODOT_VERSION_STATUS") is not None:
            status = str(os.getenv("GODOT_VERSION_STATUS"))
        v += ".%s.%s" % (status, name)
    return v


def lipo(prefix, suffix):
    from pathlib import Path

    target_bin = ""
    lipo_command = ["lipo", "-create"]
    arch_found = 0

    for arch in architectures:
        bin_name = prefix + "." + arch + suffix
        if Path(bin_name).is_file():
            target_bin = bin_name
            lipo_command += [bin_name]
            arch_found += 1

    if arch_found > 1:
        target_bin = prefix + ".fat" + suffix
        lipo_command += ["-output", target_bin]
        subprocess.run(lipo_command)

    return target_bin


def get_mvk_sdk_path(osname):
    def int_or_zero(i):
        try:
            return int(i)
        except (TypeError, ValueError):
            return 0

    def ver_parse(a):
        return [int_or_zero(i) for i in a.split(".")]

    dirname = os.path.expanduser("~/VulkanSDK")
    if not os.path.exists(dirname):
        return ""

    ver_min = ver_parse("1.3.231.0")
    ver_num = ver_parse("0.0.0.0")
    files = os.listdir(dirname)
    lib_name_out = dirname
    for file in files:
        if os.path.isdir(os.path.join(dirname, file)):
            ver_comp = ver_parse(file)
            if ver_comp > ver_num and ver_comp >= ver_min:
                lib_name = os.path.join(os.path.join(dirname, file), "macOS/lib/MoltenVK.xcframework/" + osname + "/")
                if os.path.isfile(os.path.join(lib_name, "libMoltenVK.a")):
                    ver_num = ver_comp
                    lib_name_out = os.path.join(os.path.join(dirname, file), "macOS/lib/MoltenVK.xcframework")
                else:
                    lib_name = os.path.join(
                        os.path.join(dirname, file), "MoltenVK/MoltenVK.xcframework/" + osname + "/"
                    )
                    if os.path.isfile(os.path.join(lib_name, "libMoltenVK.a")):
                        ver_num = ver_comp
                        lib_name_out = os.path.join(os.path.join(dirname, file), "MoltenVK/MoltenVK.xcframework")

    return lib_name_out


def detect_mvk(env, osname):
    mvk_list = [
        get_mvk_sdk_path(osname),
        "/opt/homebrew/Frameworks/MoltenVK.xcframework",
        "/usr/local/homebrew/Frameworks/MoltenVK.xcframework",
        "/opt/local/Frameworks/MoltenVK.xcframework",
    ]
    vk_sdk = env.get("vulkan_sdk_path", "")
    if vk_sdk != "":
        mvk_list.insert(0, os.path.expanduser(vk_sdk))
        mvk_list.insert(0, os.path.join(os.path.expanduser(vk_sdk), "macOS/lib/MoltenVK.xcframework"))
        mvk_list.insert(0, os.path.join(os.path.expanduser(vk_sdk), "MoltenVK/MoltenVK.xcframework"))

    for mvk_path in mvk_list:
        if mvk_path and os.path.isfile(os.path.join(mvk_path, f"{osname}/libMoltenVK.a")):
            print(f"MoltenVK found at: {mvk_path}")
            return mvk_path

    return ""


def combine_libs_apple_embedded(lib_path, source_libs, toolchain_path="", triple=""):
    prefix = os.path.join(toolchain_path, "usr", "bin", triple) if toolchain_path else ""
    libtool = prefix + "libtool"
    subprocess.run([libtool, "-static", "-o", str(lib_path), *[str(s) for s in source_libs]], check=True)


def lipo_and_copy_apple_embedded(
    platform, framework_dir, framework_dir_sim, rel_prefix, dbg_prefix, module_prefix, app_dir, env
):
    bin_dir = os.path.abspath("bin")
    extra_suffix = getattr(env, "extra_suffix", "").replace(".simulator", "")
    rel_target_bin = lipo(bin_dir + "/libgodot" + module_prefix + "." + rel_prefix, extra_suffix + ".a")
    dbg_target_bin = lipo(bin_dir + "/libgodot" + module_prefix + "." + dbg_prefix, extra_suffix + ".a")
    rel_target_bin_sim = lipo(
        bin_dir + "/libgodot" + module_prefix + "." + rel_prefix, ".simulator" + extra_suffix + ".a"
    )
    dbg_target_bin_sim = lipo(
        bin_dir + "/libgodot" + module_prefix + "." + dbg_prefix, ".simulator" + extra_suffix + ".a"
    )
    if rel_target_bin != "":
        print(f' Copying "{platform}" release framework')
        shutil.copy(
            rel_target_bin,
            f"{app_dir}/libgodot{module_prefix}.{platform}.release.xcframework/{framework_dir}/libgodot{module_prefix}.a",
        )
    if dbg_target_bin != "":
        print(f' Copying "{platform}" debug framework')
        shutil.copy(
            dbg_target_bin,
            f"{app_dir}/libgodot{module_prefix}.{platform}.debug.xcframework/{framework_dir}/libgodot{module_prefix}.a",
        )
    if rel_target_bin_sim != "":
        print(f' Copying "{platform}" (simulator) release framework')
        shutil.copy(
            rel_target_bin_sim,
            f"{app_dir}/libgodot{module_prefix}.{platform}.release.xcframework/{framework_dir_sim}/libgodot{module_prefix}.a",
        )
    if dbg_target_bin_sim != "":
        print(f' Copying "{platform}" (simulator) debug framework')
        shutil.copy(
            dbg_target_bin_sim,
            f"{app_dir}/libgodot{module_prefix}.{platform}.debug.xcframework/{framework_dir_sim}/libgodot{module_prefix}.a",
        )


def generate_bundle_apple_embedded(platform, framework_dir, framework_dir_sim, use_mkv, target, source, env):
    extra_suffix = getattr(env, "extra_suffix", "").replace(".simulator", "")
    app_prefix = "godot." + platform
    rel_prefix = platform + ".template_release"
    dbg_prefix = platform + ".template_debug"
    if getattr(env, "dev_build", False):
        app_prefix += ".dev"
        rel_prefix += ".dev"
        dbg_prefix += ".dev"
    if env.get("precision") == "double":
        app_prefix += ".double"
        rel_prefix += ".double"
        dbg_prefix += ".double"

    app_dir = os.path.abspath("bin/" + platform + "_xcode")
    templ = os.path.abspath("misc/dist/apple_embedded_xcode")
    if os.path.exists(app_dir):
        shutil.rmtree(app_dir)
    shutil.copytree(templ, app_dir)

    lipo_and_copy_apple_embedded(platform, framework_dir, framework_dir_sim, rel_prefix, dbg_prefix, "", app_dir, env)
    if "MODULES_EXTERNAL" in env:
        for mod in env["MODULES_EXTERNAL"]:
            lipo_and_copy_apple_embedded(
                platform, framework_dir, framework_dir_sim, rel_prefix, dbg_prefix, mod, app_dir, env
            )

    for entry in os.listdir(app_dir):
        if (entry.startswith("libgodot.") or entry.startswith("libgodot_")) and entry.endswith(".xcframework"):
            parts = entry.split(".")
            if len(parts) >= 3 and parts[1] != platform:
                shutil.rmtree(os.path.join(app_dir, entry))

    if use_mkv:
        mvk_path = detect_mvk(env, "ios-arm64")
        if mvk_path != "":
            shutil.copytree(mvk_path + "/ios-arm64", app_dir + "/MoltenVK.xcframework/ios-arm64")
            shutil.copytree(
                mvk_path + "/ios-arm64_x86_64-simulator", app_dir + "/MoltenVK.xcframework/ios-arm64_x86_64-simulator"
            )
            shutil.copy(mvk_path + "/Info.plist", app_dir + "/MoltenVK.xcframework/Info.plist")

    zip_dir = os.path.abspath("bin/" + (app_prefix + extra_suffix).replace(".", "_"))
    shutil.make_archive(zip_dir, "zip", root_dir=app_dir)
    shutil.rmtree(app_dir)

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


def generate_wayland_protocols(sowrap=True):
    scanner = shutil.which("wayland-scanner")
    proto_dir = "platform/linuxbsd/wayland/protocol"
    os.makedirs(proto_dir, exist_ok=True)
    if not scanner:
        return

    h_chunks = ["#pragma once\n"]
    c_chunks = ["/* THIS FILE IS GENERATED. EDITS WILL BE LOST. */\n"]
    wrap_inc = "platform/linuxbsd/xkbcommon-so_wrap.h"

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


def version_info_builder(target, source, modules):
    with methods.generated_wrapper(str(target[0])) as file:
        file.write("""\
#define VLTR_VERSION_SHORT_NAME "{short_name}"
#define VLTR_VERSION_NAME "{name}"
#define VLTR_VERSION_MAJOR {major}
#define VLTR_VERSION_MINOR {minor}
#define VLTR_VERSION_PATCH {patch}
#define VLTR_VERSION_STATUS "{status}"
#define VLTR_VERSION_BUILD "{build}"
#define VLTR_VERSION_MODULE_CONFIG "{module_config}"
#define VLTR_VERSION_WEBSITE "{website}"
#define VLTR_VERSION_DOCS_BRANCH "{docs_branch}"
#define VLTR_VERSION_DOCS_URL "https://docs.godotengine.org/en/" VLTR_VERSION_DOCS_BRANCH

""".format(**source[0]))
        for module in sorted(modules.keys()):
            upper = module.upper()
            file.write(f"#ifndef MODULE_{upper}_DISABLED\n#define MODULE_{upper}_ENABLED\n#endif\n")


def make_icu_data(target, source):
    buffer = methods.get_buffer(str(source[0]))
    with methods.generated_wrapper(str(target[0])) as file:
        file.write(f"""\
/* (C) 2016 and later: Unicode, Inc. and others. */
/* License & terms of use: https://www.unicode.org/copyright.html */

#include <unicode/utypes.h>
#include <unicode/udata.h>
#include <unicode/uversion.h>

extern "C" U_EXPORT const size_t U_ICUDATA_SIZE = {len(buffer)};
extern "C" U_EXPORT const unsigned char U_ICUDATA_ENTRY_POINT[] = {{
{methods.format_buffer(buffer, 1)}
}};
""")


class RDHeaderStruct:
    def __init__(self):
        self.vertex_lines = []
        self.fragment_lines = []
        self.compute_lines = []
        self.raygen_lines = []
        self.any_hit_lines = []
        self.closest_hit_lines = []
        self.miss_lines = []
        self.intersection_lines = []

        self.vertex_included_files = []
        self.fragment_included_files = []
        self.compute_included_files = []
        self.raygen_included_files = []
        self.any_hit_included_files = []
        self.closest_hit_included_files = []
        self.miss_included_files = []
        self.intersection_included_files = []

        self.reading = ""
        self.line_offset = 0
        self.vertex_offset = 0
        self.fragment_offset = 0
        self.compute_offset = 0
        self.raygen_offset = 0
        self.any_hit_offset = 0
        self.closest_hit_offset = 0
        self.miss_offset = 0
        self.intersection_offset = 0


def include_file_in_rd_header(filename: str, header_data: RDHeaderStruct, depth: int) -> RDHeaderStruct:
    with open(filename, "r", encoding="utf-8") as fs:
        line = fs.readline()

        while line:
            index = line.find("//")
            if index != -1:
                line = line[:index]

            if line.find("#[vertex]") != -1:
                header_data.reading = "vertex"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.vertex_offset = header_data.line_offset
                continue

            if line.find("#[fragment]") != -1:
                header_data.reading = "fragment"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.fragment_offset = header_data.line_offset
                continue

            if line.find("#[compute]") != -1:
                header_data.reading = "compute"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.compute_offset = header_data.line_offset
                continue

            if line.find("#[raygen]") != -1:
                header_data.reading = "raygen"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.raygen_offset = header_data.line_offset
                continue

            if line.find("#[any_hit]") != -1:
                header_data.reading = "any_hit"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.any_hit_offset = header_data.line_offset
                continue

            if line.find("#[closest_hit]") != -1:
                header_data.reading = "closest_hit"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.closest_hit_offset = header_data.line_offset
                continue

            if line.find("#[miss]") != -1:
                header_data.reading = "miss"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.miss_offset = header_data.line_offset
                continue

            if line.find("#[intersection]") != -1:
                header_data.reading = "intersection"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.intersection_offset = header_data.line_offset
                continue

            while line.find("#include ") != -1:
                includeline = line.replace("#include ", "").strip()[1:-1]

                if includeline.startswith("thirdparty/"):
                    included_file = os.path.relpath(includeline)

                else:
                    included_file = os.path.relpath(os.path.dirname(filename) + "/" + includeline)

                if included_file not in header_data.vertex_included_files and header_data.reading == "vertex":
                    header_data.vertex_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif included_file not in header_data.fragment_included_files and header_data.reading == "fragment":
                    header_data.fragment_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif included_file not in header_data.compute_included_files and header_data.reading == "compute":
                    header_data.compute_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif included_file not in header_data.raygen_included_files and header_data.reading == "raygen":
                    header_data.raygen_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif included_file not in header_data.any_hit_included_files and header_data.reading == "any_hit":
                    header_data.any_hit_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif (
                    included_file not in header_data.closest_hit_included_files and header_data.reading == "closest_hit"
                ):
                    header_data.closest_hit_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif included_file not in header_data.miss_included_files and header_data.reading == "miss":
                    header_data.miss_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif (
                    included_file not in header_data.intersection_included_files
                    and header_data.reading == "intersection"
                ):
                    header_data.intersection_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')

                line = fs.readline()

            line = line.replace("\r", "").replace("\n", "")

            if header_data.reading == "vertex":
                header_data.vertex_lines += [line]
            if header_data.reading == "fragment":
                header_data.fragment_lines += [line]
            if header_data.reading == "compute":
                header_data.compute_lines += [line]
            if header_data.reading == "raygen":
                header_data.raygen_lines += [line]
            if header_data.reading == "any_hit":
                header_data.any_hit_lines += [line]
            if header_data.reading == "closest_hit":
                header_data.closest_hit_lines += [line]
            if header_data.reading == "miss":
                header_data.miss_lines += [line]
            if header_data.reading == "intersection":
                header_data.intersection_lines += [line]

            line = fs.readline()
            header_data.line_offset += 1

    return header_data


def build_rd_header_lines_for_raytracing_stage(lines, stage: str):
    if lines:
        return f"""\
	static const char _{stage}_code[] = {{
{to_raw_cstring(lines)}
	}};
"""
    else:
        return f"""\
	static const char *_{stage}_code = nullptr;
"""


def build_rd_shader_body(shader: str) -> str:
    include_file_in_rd_header(shader, header_data := RDHeaderStruct(), 0)
    class_name = os.path.basename(shader).replace(".glsl", "").title().replace("_", "").replace(".", "") + "ShaderRD"

    out = [f"inline ShaderRD::StageData {class_name}() {{\n"]
    if (
        header_data.raygen_lines
        or header_data.any_hit_lines
        or header_data.closest_hit_lines
        or header_data.miss_lines
        or header_data.intersection_lines
    ):
        out.append(build_rd_header_lines_for_raytracing_stage(header_data.raygen_lines, "raygen"))
        out.append(build_rd_header_lines_for_raytracing_stage(header_data.any_hit_lines, "any_hit"))
        out.append(build_rd_header_lines_for_raytracing_stage(header_data.closest_hit_lines, "closest_hit"))
        out.append(build_rd_header_lines_for_raytracing_stage(header_data.miss_lines, "miss"))
        out.append(build_rd_header_lines_for_raytracing_stage(header_data.intersection_lines, "intersection"))
        out.append(f"""\
	ShaderRD::StageData d;
	d.raygen_code = _raygen_code;
	d.any_hit_code = _any_hit_code;
	d.closest_hit_code = _closest_hit_code;
	d.miss_code = _miss_code;
	d.intersection_code = _intersection_code;
	d.name = "{class_name}";
	d.is_raytracing = true;
	return d;
""")
    elif header_data.compute_lines:
        out.append(f"""\
	static const char _compute_code[] = {{
{to_raw_cstring(header_data.compute_lines)}
	}};
	ShaderRD::StageData d;
	d.compute_code = _compute_code;
	d.name = "{class_name}";
	return d;
""")
    else:
        out.append(f"""\
	static const char _vertex_code[] = {{
{to_raw_cstring(header_data.vertex_lines)}
	}};
	static const char _fragment_code[] = {{
{to_raw_cstring(header_data.fragment_lines)}
	}};
	ShaderRD::StageData d;
	d.vertex_code = _vertex_code;
	d.fragment_code = _fragment_code;
	d.name = "{class_name}";
	return d;
""")
    out.append("}\n")
    return "".join(out)


def build_rd_header(filename: str, shader: str) -> None:
    with generated_wrapper(filename) as file:
        file.write('#include "core/types.h"\n#include "servers/rendering/renderer_rd/shader_rd.h"\n\n')
        file.write(build_rd_shader_body(shader))


class RAWHeaderStruct:
    def __init__(self):
        self.code = ""


def include_file_in_raw_header(filename: str, header_data: RAWHeaderStruct, depth: int) -> None:
    with open(filename, "r", encoding="utf-8") as fs:
        line = fs.readline()

        while line:
            while line.find("#include ") != -1:
                includeline = line.replace("#include ", "").strip()[1:-1]
                if includeline.startswith("thirdparty/"):
                    included_file = os.path.relpath(includeline)
                else:
                    included_file = os.path.relpath(os.path.dirname(filename) + "/" + includeline)
                include_file_in_raw_header(included_file, header_data, depth + 1)

                line = fs.readline()

            header_data.code += line
            line = fs.readline()


def build_raw_shader_body(shader: str) -> str:
    include_file_in_raw_header(shader, header_data := RAWHeaderStruct(), 0)
    return f"""\
inline constexpr const char {os.path.basename(shader).replace(".glsl", "_shader_glsl")}[] = {{
{to_raw_cstring(header_data.code)}
}};
"""


def build_raw_header(filename: str, shader: str) -> None:
    with generated_wrapper(filename) as file:
        file.write('#include "core/types.h"\n\n')
        file.write(build_raw_shader_body(shader))


def build_raw_headers_bundle(filename: str, shaders: list[str]) -> None:
    with generated_wrapper(filename) as file:
        file.write('#include "core/types.h"\n\n')
        for idx, shader in enumerate(shaders):
            if idx:
                file.write("\n")
            file.write(build_raw_shader_body(shader))


def build_raw_headers(target, source, env):
    env.NoCache(target)
    for src in source:
        build_raw_header(f"{src}.gen.h", str(src))


class GLES3HeaderStruct:
    def __init__(self):
        self.vertex_lines = []
        self.fragment_lines = []
        self.raygen_lines = []
        self.any_hit_lines = []
        self.closest_hit_lines = []
        self.miss_lines = []
        self.intersection_lines = []
        self.uniforms = []
        self.fbos = []
        self.texunits = []
        self.texunit_names = []
        self.ubos = []
        self.ubo_names = []
        self.feedbacks = []

        self.vertex_included_files = []
        self.fragment_included_files = []

        self.reading = ""
        self.line_offset = 0
        self.vertex_offset = 0
        self.fragment_offset = 0
        self.raygen_offset = 0
        self.any_hit_offset = 0
        self.closest_hit_offset = 0
        self.miss_offset = 0
        self.intersection_offset = 0
        self.variant_defines = []
        self.variant_names = []
        self.specialization_names = []
        self.specialization_values = []


def include_file_in_gles3_header(filename: str, header_data: GLES3HeaderStruct, depth: int):
    with open(filename, "r", encoding="utf-8") as fs:
        line = fs.readline()

        while line:
            if line.find("=") != -1 and header_data.reading == "":
                # Mode
                eqpos = line.find("=")
                defname = line[:eqpos].strip().upper()
                define = line[eqpos + 1 :].strip()
                header_data.variant_names.append(defname)
                header_data.variant_defines.append(define)
                line = fs.readline()
                header_data.line_offset += 1
                header_data.vertex_offset = header_data.line_offset
                continue

            if line.find("=") != -1 and header_data.reading == "specializations":
                # Specialization
                eqpos = line.find("=")
                specname = line[:eqpos].strip()
                specvalue = line[eqpos + 1 :]
                header_data.specialization_names.append(specname)
                header_data.specialization_values.append(specvalue)
                line = fs.readline()
                header_data.line_offset += 1
                header_data.vertex_offset = header_data.line_offset
                continue

            if line.find("#[modes]") != -1:
                # Nothing really, just skip
                line = fs.readline()
                header_data.line_offset += 1
                header_data.vertex_offset = header_data.line_offset
                continue

            if line.find("#[specializations]") != -1:
                header_data.reading = "specializations"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.vertex_offset = header_data.line_offset
                continue

            if line.find("#[vertex]") != -1:
                header_data.reading = "vertex"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.vertex_offset = header_data.line_offset
                continue

            if line.find("#[fragment]") != -1:
                header_data.reading = "fragment"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.fragment_offset = header_data.line_offset
                continue

            if line.find("#[raygen]") != -1:
                header_data.reading = "raygen"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.raygen_offset = header_data.line_offset
                continue

            if line.find("#[any_hit]") != -1:
                header_data.reading = "any_hit"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.any_hit_offset = header_data.line_offset
                continue

            if line.find("#[closest_hit]") != -1:
                header_data.reading = "closest_hit"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.closest_hit_offset = header_data.line_offset
                continue

            if line.find("#[miss]") != -1:
                header_data.reading = "miss"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.miss_offset = header_data.line_offset
                continue

            if line.find("#[intersection]") != -1:
                header_data.reading = "intersection"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.intersection_offset = header_data.line_offset
                continue

            while line.find("#include ") != -1:
                includeline = line.replace("#include ", "").strip()[1:-1]

                included_file = os.path.relpath(os.path.dirname(filename) + "/" + includeline)
                if included_file not in header_data.vertex_included_files and header_data.reading == "vertex":
                    header_data.vertex_included_files += [included_file]
                    if include_file_in_gles3_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif included_file not in header_data.fragment_included_files and header_data.reading == "fragment":
                    header_data.fragment_included_files += [included_file]
                    if include_file_in_gles3_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')

                line = fs.readline()

            if line.find("uniform") != -1 and line.lower().find("texunit:") != -1:
                # texture unit
                texunitstr = line[line.find(":") + 1 :].strip()
                if texunitstr == "auto":
                    texunit = "-1"
                else:
                    texunit = str(int(texunitstr))
                uline = line[: line.lower().find("//")]
                uline = uline.replace("uniform", "")
                uline = uline.replace("highp", "")
                uline = uline.replace(";", "")
                lines = uline.split(",")
                for x in lines:
                    x = x.strip()
                    x = x[x.rfind(" ") + 1 :]
                    if x.find("[") != -1:
                        # unfiorm array
                        x = x[: x.find("[")]

                    if x not in header_data.texunit_names:
                        header_data.texunits += [(x, texunit)]
                        header_data.texunit_names += [x]

            elif line.find("uniform") != -1 and line.lower().find("ubo:") != -1:
                # uniform buffer object
                ubostr = line[line.find(":") + 1 :].strip()
                ubo = str(int(ubostr))
                uline = line[: line.lower().find("//")]
                uline = uline[uline.find("uniform") + len("uniform") :]
                uline = uline.replace("highp", "")
                uline = uline.replace(";", "")
                uline = uline.replace("{", "").strip()
                lines = uline.split(",")
                for x in lines:
                    x = x.strip()
                    x = x[x.rfind(" ") + 1 :]
                    if x.find("[") != -1:
                        # unfiorm array
                        x = x[: x.find("[")]

                    if x not in header_data.ubo_names:
                        header_data.ubos += [(x, ubo)]
                        header_data.ubo_names += [x]

            elif line.find("uniform") != -1 and line.find("{") == -1 and line.find(";") != -1:
                uline = line.replace("uniform", "")
                uline = uline.replace(";", "")
                lines = uline.split(",")
                for x in lines:
                    x = x.strip()
                    x = x[x.rfind(" ") + 1 :]
                    if x.find("[") != -1:
                        # unfiorm array
                        x = x[: x.find("[")]

                    if x not in header_data.uniforms:
                        header_data.uniforms += [x]

            if (line.strip().find("out ") == 0 or line.strip().find("flat ") == 0) and line.find("tfb:") != -1:
                uline = line.replace("flat ", "")
                uline = uline.replace("out ", "")
                uline = uline.replace("highp ", "")
                uline = uline.replace(";", "")
                uline = uline[uline.find(" ") :].strip()

                if uline.find("//") != -1:
                    name, bind = uline.split("//")
                    if bind.find("tfb:") != -1:
                        name = name.strip()
                        bind = bind.replace("tfb:", "").strip()
                        header_data.feedbacks += [(name, bind)]

            line = line.replace("\r", "")
            line = line.replace("\n", "")

            if header_data.reading == "vertex":
                header_data.vertex_lines += [line]
            if header_data.reading == "fragment":
                header_data.fragment_lines += [line]
            if header_data.reading == "raygen":
                header_data.raygen_lines += [line]
            if header_data.reading == "any_hit":
                header_data.any_hit_lines += [line]
            if header_data.reading == "closest_hit":
                header_data.closest_hit_lines += [line]
            if header_data.reading == "miss":
                header_data.miss_lines += [line]
            if header_data.reading == "intersection":
                header_data.intersection_lines += [line]

            line = fs.readline()
            header_data.line_offset += 1

    return header_data


from io import StringIO


def _write_gles3_struct(file, shader: str) -> None:
    include_file_in_gles3_header(shader, header_data := GLES3HeaderStruct(), 0)
    out_file_class = (
        os.path.basename(shader).replace(".glsl", "").title().replace("_", "").replace(".", "") + "ShaderGLES3"
    )

    if True:
        defspec = 0
        defvariant = ""

        file.write(f"""\
struct {out_file_class} {{
	using TextureUniformData = ShaderGLES3::TextureUniformData;

	ShaderGLES3 shader;

""")

        if header_data.uniforms:
            uniforms = ",\n\t\t".join(uniform.upper() for uniform in header_data.uniforms)
            file.write(f"""\
	enum Uniforms {{
		{uniforms},
	}};

""")

        if header_data.variant_names:
            variant_names = ",\n\t\t".join(name for name in header_data.variant_names)
        else:
            variant_names = "DEFAULT"
            defvariant = " = DEFAULT"
        file.write(f"""\
	enum ShaderVariant {{
		{variant_names},
	}};

""")

        if header_data.specialization_names:
            specialization_names = ",\n\t\t".join(
                f"{name.upper()} = {1 << index}" for index, name in enumerate(header_data.specialization_names)
            )
            file.write(f"""\
	enum Specializations {{
		{specialization_names},
	}};

""")
        for index, specialization_value in enumerate(header_data.specialization_values):
            if specialization_value.strip().upper() in ["TRUE", "1"]:
                defspec |= 1 << index

        file.write(f"""\
	_FORCE_INLINE_ bool version_bind_shader(RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		return shader._version_bind_shader(p_version, p_variant, p_specialization);
	}}

""")

        if header_data.uniforms:
            file.write(f"""\
	_FORCE_INLINE_ int version_get_uniform(Uniforms p_uniform, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		return shader._version_get_uniform(p_uniform, p_version, p_variant, p_specialization);
	}}

	/* clang-format off */
#define TRY_GET_UNIFORM(var_name) int var_name = version_get_uniform(p_uniform, p_version, p_variant, p_specialization); if (var_name < 0) return
	/* clang-format on */

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, float p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1f(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, double p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1f(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, uint8_t p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1ui(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, int8_t p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1i(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, uint16_t p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1ui(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, int16_t p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1i(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, uint32_t p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1ui(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, int32_t p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1i(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Color &p_color, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		GLfloat col[4] = {{ p_color.r, p_color.g, p_color.b, p_color.a }};
		glUniform4fv(uniform_location, 1, col);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Vector2 &p_vec2, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		GLfloat vec2[2] = {{ float(p_vec2.x), float(p_vec2.y) }};
		glUniform2fv(uniform_location, 1, vec2);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Size2i &p_vec2, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		GLint vec2[2] = {{ GLint(p_vec2.x), GLint(p_vec2.y) }};
		glUniform2iv(uniform_location, 1, vec2);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Vector3 &p_vec3, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		GLfloat vec3[3] = {{ float(p_vec3.x), float(p_vec3.y), float(p_vec3.z) }};
		glUniform3fv(uniform_location, 1, vec3);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Vector4 &p_vec4, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		GLfloat vec4[4] = {{ float(p_vec4.x), float(p_vec4.y), float(p_vec4.z), float(p_vec4.w) }};
		glUniform4fv(uniform_location, 1, vec4);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, float p_a, float p_b, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform2f(uniform_location, p_a, p_b);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, float p_a, float p_b, float p_c, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform3f(uniform_location, p_a, p_b, p_c);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, float p_a, float p_b, float p_c, float p_d, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform4f(uniform_location, p_a, p_b, p_c, p_d);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Transform3D &p_transform, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		const Transform3D &tr = p_transform;

		GLfloat matrix[16] = {{ /* build a 16x16 matrix */
			(GLfloat)tr.basis.rows[0][0],
			(GLfloat)tr.basis.rows[1][0],
			(GLfloat)tr.basis.rows[2][0],
			(GLfloat)0,
			(GLfloat)tr.basis.rows[0][1],
			(GLfloat)tr.basis.rows[1][1],
			(GLfloat)tr.basis.rows[2][1],
			(GLfloat)0,
			(GLfloat)tr.basis.rows[0][2],
			(GLfloat)tr.basis.rows[1][2],
			(GLfloat)tr.basis.rows[2][2],
			(GLfloat)0,
			(GLfloat)tr.origin.x,
			(GLfloat)tr.origin.y,
			(GLfloat)tr.origin.z,
			(GLfloat)1
		}};

		glUniformMatrix4fv(uniform_location, 1, false, matrix);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Transform2D &p_transform, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		const Transform2D &tr = p_transform;

		GLfloat matrix[16] = {{ /* build a 16x16 matrix */
			(GLfloat)tr.columns[0][0],
			(GLfloat)tr.columns[0][1],
			(GLfloat)0,
			(GLfloat)0,
			(GLfloat)tr.columns[1][0],
			(GLfloat)tr.columns[1][1],
			(GLfloat)0,
			(GLfloat)0,
			(GLfloat)0,
			(GLfloat)0,
			(GLfloat)1,
			(GLfloat)0,
			(GLfloat)tr.columns[2][0],
			(GLfloat)tr.columns[2][1],
			(GLfloat)0,
			(GLfloat)1
		}};

		glUniformMatrix4fv(uniform_location, 1, false, matrix);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Projection &p_matrix, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		GLfloat matrix[16];

		for (int i = 0; i < 4; i++) {{
			for (int j = 0; j < 4; j++) {{
				matrix[i * 4 + j] = p_matrix.columns[i][j];
			}}
		}}

		glUniformMatrix4fv(uniform_location, 1, false, matrix);
	}}

#undef TRY_GET_UNIFORM

""")

        file.write("""\
	RID version_create() {
		return shader.version_create();
	}

	void version_set_code(RID p_version, const HashMap<String, String> &p_code, const String &p_uniforms, const String &p_vertex_globals, const String &p_fragment_globals, const Vector<String> &p_custom_defines, const LocalVector<ShaderGLES3::TextureUniformData> &p_texture_uniforms, bool p_initialize = false) {
		shader.version_set_code(p_version, p_code, p_uniforms, p_vertex_globals, p_fragment_globals, p_custom_defines, p_texture_uniforms, p_initialize);
	}

	bool version_is_valid(RID p_version) {
		return shader.version_is_valid(p_version);
	}

	bool version_free(RID p_version) {
		return shader.version_free(p_version);
	}

	RenderingServerTypes::ShaderNativeSourceCode version_get_native_source_code(RID p_version) {
		return shader.version_get_native_source_code(p_version);
	}

	void initialize(const String &p_general_defines = "", int p_base_texture_index = 0) {
		_init();
		shader.initialize(p_general_defines, p_base_texture_index);
	}

	void _init() {
""")

        if header_data.uniforms:
            uniforms = ",\n\t\t\t".join(f'"{uniform}"' for uniform in header_data.uniforms)
            file.write(f"""\
		static const char *_uniform_strings[] = {{
			{uniforms}
		}};
""")
        else:
            file.write("""\
		static const char **_uniform_strings = nullptr;
""")

        if header_data.variant_defines:
            variant_count = len(header_data.variant_defines)
            variant_defines = ",\n\t\t\t".join(f'"{define}"' for define in header_data.variant_defines)
            file.write(f"""\
		static const char *_variant_defines[] = {{
			{variant_defines},
		}};
""")
        else:
            variant_count = 1
            file.write("""\
		static const char *_variant_defines[] = {" "};
""")

        if header_data.texunits:
            texunits = ",\n\t\t\t".join(f'{{ "{name}", {texunit} }}' for name, texunit in header_data.texunits)
            file.write(f"""\
		static ShaderGLES3::TexUnitPair _texunit_pairs[] = {{
			{texunits},
		}};
""")
        else:
            file.write("""\
		static ShaderGLES3::TexUnitPair *_texunit_pairs = nullptr;
""")

        if header_data.ubos:
            ubos = ",\n\t\t\t".join(f'{{ "{name}", {ubo} }}' for name, ubo in header_data.ubos)
            file.write(f"""\
		static ShaderGLES3::UBOPair _ubo_pairs[] = {{
			{ubos},
		}};
""")
        else:
            file.write("""\
		static ShaderGLES3::UBOPair *_ubo_pairs = nullptr;
""")

        if header_data.specialization_names:
            specializations = ",\n\t\t\t".join(
                f'{{ "{name}", {"true" if header_data.specialization_values[index].strip().upper() in ["TRUE", "1"] else "false"} }}'
                for index, name in enumerate(header_data.specialization_names)
            )
            file.write(f"""\
		static ShaderGLES3::Specialization _spec_pairs[] = {{
			{specializations},
		}};
""")
        else:
            file.write("""\
		static ShaderGLES3::Specialization *_spec_pairs = nullptr;
""")

        if header_data.feedbacks:
            feedbacks = ",\n\t\t\t".join(
                f'{{ "{name}", {0 if spec not in header_data.specialization_names else (1 << header_data.specialization_names.index(spec))} }}'
                for name, spec in header_data.feedbacks
            )
            file.write(f"""\
		static const ShaderGLES3::Feedback _feedbacks[] = {{
			{feedbacks},
		}};
""")
        else:
            file.write("""\
		static const ShaderGLES3::Feedback *_feedbacks = nullptr;
""")

        file.write(f"""\
		static const char _vertex_code[] = {{
{to_raw_cstring(header_data.vertex_lines)}
		}};

		static const char _fragment_code[] = {{
{to_raw_cstring(header_data.fragment_lines)}
		}};

		shader._setup(_vertex_code, _fragment_code, "{out_file_class}",
				{len(header_data.uniforms)}, _uniform_strings, {len(header_data.ubos)}, _ubo_pairs,
				{len(header_data.feedbacks)}, _feedbacks, {len(header_data.texunits)}, _texunit_pairs,
				{len(header_data.specialization_names)}, _spec_pairs, {variant_count}, _variant_defines);
	}}
}};
""")


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


def _write_ltc_dds_bytes(file, source_path):
    with open(source_path, mode="rb") as binary:
        buffer = binary.read()[148:]  # skip .dds header
        file.write("0x{:02x}".format(buffer[0]))
        for byte in range(1, len(buffer)):
            file.write(",0x{:02x}".format(buffer[byte]))


def _write_controller_mappings(file, mapping_sources):
    platform_variables = {
        "Linux": "LINUXBSD",
        "Windows": "WINDOWS",
        "Mac OS X": "MACOS",
        "Android": "ANDROID",
        "iOS": "APPLE_EMBEDDED",
        "Web": "WEB",
    }
    platform_mappings = OrderedDict()
    for src_path in mapping_sources:
        with open(src_path, "r", encoding="utf-8") as f:
            mapping_file_lines = f.readlines()

        current_platform = None
        for line in mapping_file_lines:
            line = line.strip()
            if not line:
                continue
            if line[0] == "#":
                platform_or_header = line[1:].strip()
                if platform_or_header not in platform_variables:
                    continue
                current_platform = platform_or_header
                if current_platform not in platform_mappings:
                    platform_mappings[current_platform] = {}
            elif current_platform:
                guid = line.split(",")[0]
                if guid in platform_mappings[current_platform]:
                    file.write(
                        "// WARNING: DATABASE {} OVERWROTE PRIOR MAPPING: {} {}\n".format(
                            src_path,
                            current_platform,
                            platform_mappings[current_platform][guid],
                        )
                    )
                platform_mappings[current_platform][guid] = line

    file.write("const char *DefaultControllerMappings::mappings[] = {\n")
    for platform, mappings in platform_mappings.items():
        variable = platform_variables[platform]
        file.write(f"#ifdef {variable}_ENABLED\n")
        for mapping in mappings.values():
            file.write(f'\t"{mapping}",\n')
        file.write(f"#endif // {variable}_ENABLED\n")
    file.write("\tnullptr\n};\n")


def _write_license_parts(file, source):
    src_copyright = str(source[0])
    src_license = str(source[1])

    class LicenseReader:
        def __init__(self, license_file: TextIOWrapper):
            self._license_file = license_file
            self.line_num = 0
            self.current = self.next_line()

        def next_line(self):
            line = self._license_file.readline()
            self.line_num += 1
            while line.startswith("#"):
                line = self._license_file.readline()
                self.line_num += 1
            self.current = line
            return line

        def next_tag(self):
            if ":" not in self.current:
                return ("", [])
            tag, line = self.current.split(":", 1)
            lines = [line.strip()]
            while self.next_line() and self.current.startswith(" "):
                lines.append(self.current.strip())
            return (tag, lines)

    projects = OrderedDict()
    license_list = []

    with open(src_copyright, "r", encoding="utf-8") as copyright_file:
        reader = LicenseReader(copyright_file)
        part = {}
        while reader.current:
            tag, content = reader.next_tag()
            if tag in ("Files", "Copyright", "License"):
                part[tag] = content[:]
            elif tag == "Comment" and part:
                projects[content[0]] = projects.get(content[0], []) + [part]

            if not tag or not reader.current:
                if "License" in part and "Files" not in part:
                    license_list.append(part["License"])
                part = {}
                reader.next_line()

    data_list = []
    for project in iter(projects.values()):
        for part in project:
            part["file_index"] = len(data_list)
            data_list += part["Files"]
            part["copyright_index"] = len(data_list)
            data_list += part["Copyright"]

    with open(src_license, "r", encoding="utf-8") as lic_f:
        license_text = lic_f.read()

    authors_sections = {
        "Project Founders": "AUTHORS_FOUNDERS",
        "Lead Developer": "AUTHORS_LEAD_DEVELOPERS",
        "Project Manager": "AUTHORS_PROJECT_MANAGERS",
        "Developers": "AUTHORS_DEVELOPERS",
    }
    donors_sections = {
        "Patrons": "DONORS_PATRONS",
        "Platinum sponsors": "DONORS_SPONSORS_PLATINUM",
        "Gold sponsors": "DONORS_SPONSORS_GOLD",
        "Silver sponsors": "DONORS_SPONSORS_SILVER",
        "Diamond members": "DONORS_MEMBERS_DIAMOND",
        "Titanium members": "DONORS_MEMBERS_TITANIUM",
        "Platinum members": "DONORS_MEMBERS_PLATINUM",
        "Gold members": "DONORS_MEMBERS_GOLD",
    }

    def write_md_sections(out_file, md_path, section_map):
        buf = methods.get_buffer(md_path)
        reading = False
        for ln in buf.decode().splitlines():
            if ln.startswith("    ") and reading:
                out_file.write(f'\t"{methods.to_escaped_cstring(ln).strip()}",\n')
            elif ln.startswith("## "):
                if reading:
                    out_file.write("\tnullptr,\n};\n\n")
                    reading = False
                sec = section_map.get(ln[3:].strip())
                if sec:
                    out_file.write(f"inline constexpr const char *{sec}[] = {{\n")
                    reading = True
        if reading:
            out_file.write("\tnullptr,\n};\n\n")

    certs_buf = methods.get_buffer("thirdparty/certs/ca-bundle.crt")
    certs_decomp_size = len(certs_buf)
    certs_comp = methods.compress_buffer(certs_buf)
    area_buf = methods.get_buffer("thirdparty/smaa/AreaTex.png")
    search_buf = methods.get_buffer("thirdparty/smaa/SearchTex.png")

    enc_hex = os.environ.get("SCRIPT_AES256_ENCRYPTION_KEY") or ("0" * 64)
    enc_buf = bytes.fromhex(enc_hex)
    if len(enc_buf) != 32:
        raise ValueError("Invalid SCRIPT_AES256_ENCRYPTION_KEY length")
    file.write('#define _SYSTEM_CERTS_PATH ""\n#define BUILTIN_CERTS_ENABLED\n\n')
    file.write(f"inline constexpr int _certs_compressed_size = {len(certs_comp)};\n")
    file.write(f"inline constexpr int _certs_uncompressed_size = {certs_decomp_size};\n")
    file.write(f"inline constexpr unsigned char _certs_compressed[] = {{\n{methods.format_buffer(certs_comp, 1)}\n}};\n\n")
    write_md_sections(file, "AUTHORS.md", authors_sections)
    write_md_sections(file, "DONORS.md", donors_sections)
    file.write(f"""\
inline constexpr const char *GODOT_LICENSE_TEXT = {{
{methods.to_raw_cstring(license_text)}
}};

struct ComponentCopyrightPart {{
	const char *license;
	const char *const *files;
	const char *const *copyright_statements;
	int file_count;
	int copyright_count;
}};

struct ComponentCopyright {{
	const char *name;
	const ComponentCopyrightPart *parts;
	int part_count;
}};

""")

    file.write("inline constexpr const char *COPYRIGHT_INFO_DATA[] = {\n")
    for line in data_list:
        file.write(f'\t"{methods.to_escaped_cstring(line)}",\n')
    file.write("};\n\n")

    file.write("inline constexpr ComponentCopyrightPart COPYRIGHT_PROJECT_PARTS[] = {\n")
    part_index = 0
    part_indexes = {}
    for project_name, project in iter(projects.items()):
        part_indexes[project_name] = part_index
        for part in project:
            file.write(
                f'\t{{ "{methods.to_escaped_cstring(part["License"][0])}", '
                + f"&COPYRIGHT_INFO_DATA[{part['file_index']}], "
                + f"&COPYRIGHT_INFO_DATA[{part['copyright_index']}], "
                + f"{len(part['Files'])}, {len(part['Copyright'])} }},\n"
            )
            part_index += 1
    file.write("};\n\n")

    file.write(f"inline constexpr int COPYRIGHT_INFO_COUNT = {len(projects)};\n")
    file.write("inline constexpr ComponentCopyright COPYRIGHT_INFO[] = {\n")
    for project_name, project in iter(projects.items()):
        file.write(
            f'\t{{ "{methods.to_escaped_cstring(project_name)}", '
            + f"&COPYRIGHT_PROJECT_PARTS[{part_indexes[project_name]}], "
            + f"{len(project)} }},\n"
        )
    file.write("};\n\n")

    file.write(f"inline constexpr int LICENSE_COUNT = {len(license_list)};\n")
    file.write("inline constexpr const char *LICENSE_NAMES[] = {\n")
    for license in license_list:
        file.write(f'\t"{methods.to_escaped_cstring(license[0])}",\n')
    file.write("};\n\n")

    file.write("inline constexpr const char *LICENSE_BODIES[] = {\n\n")
    for license in license_list:
        to_raw = ["" if line == "." else line for line in license[1:]]
        file.write(f"{methods.to_raw_cstring(to_raw)},\n\n")
    file.write("};\n\n")

    file.write("inline constexpr int LTC_LUT_DIMENSIONS = 64;\n")
    file.write("inline constexpr uint8_t LTC_LUT1[] = {")
    _write_ltc_dds_bytes(file, "servers/rendering/storage/ltc/ltc_lut1.dds")
    file.write("};\n\n")
    file.write("inline constexpr uint8_t LTC_LUT2[] = {")
    _write_ltc_dds_bytes(file, "servers/rendering/storage/ltc/ltc_lut2.dds")
    file.write("};\n\n")
    file.write(f"""\
#define AREATEX_WIDTH 160
#define AREATEX_HEIGHT 560
#define AREATEX_PITCH (AREATEX_WIDTH * 2)
#define AREATEX_SIZE (AREATEX_HEIGHT * AREATEX_PITCH)

inline constexpr const unsigned char area_tex_png[] = {{
{methods.format_buffer(area_buf, 1)}
}};

#define SEARCHTEX_WIDTH 64
#define SEARCHTEX_HEIGHT 16
#define SEARCHTEX_PITCH SEARCHTEX_WIDTH
#define SEARCHTEX_SIZE (SEARCHTEX_HEIGHT * SEARCHTEX_PITCH)

inline constexpr const unsigned char search_tex_png[] = {{
{methods.format_buffer(search_buf, 1)}
}};
""")


EXPORT_ICONS = [
    "platform/android/export/logo.svg",
    "platform/android/export/run_icon.svg",
    "platform/ios/export/logo.svg",
    "platform/ios/export/run_icon.svg",
    "platform/linuxbsd/export/logo.svg",
    "platform/linuxbsd/export/run_icon.svg",
    "platform/macos/export/logo.svg",
    "platform/macos/export/run_icon.svg",
    "platform/visionos/export/logo.svg",
    "platform/visionos/export/run_icon.svg",
    "platform/web/export/logo.svg",
    "platform/web/export/run_icon.svg",
    "platform/windows/export/logo.svg",
    "platform/windows/export/run_icon.svg",
]

THEME_ICONS = [
    "scene/theme/icons/action_copy.svg", "scene/theme/icons/add.svg", "scene/theme/icons/arrow_down.svg",
    "scene/theme/icons/arrow_left.svg", "scene/theme/icons/arrow_right.svg", "scene/theme/icons/arrow_up.svg",
    "scene/theme/icons/bookmark.svg", "scene/theme/icons/breakpoint.svg", "scene/theme/icons/checked.svg",
    "scene/theme/icons/checked_disabled.svg", "scene/theme/icons/clear.svg", "scene/theme/icons/close.svg",
    "scene/theme/icons/close_hl.svg", "scene/theme/icons/color_picker_bar_arrow.svg", "scene/theme/icons/color_picker_cursor.svg",
    "scene/theme/icons/color_picker_cursor_bg.svg", "scene/theme/icons/color_picker_overbright.svg", "scene/theme/icons/color_picker_pipette.svg",
    "scene/theme/icons/error_icon.svg", "scene/theme/icons/favorite.svg", "scene/theme/icons/file.svg",
    "scene/theme/icons/file_mode_list.svg", "scene/theme/icons/file_mode_thumbnail.svg", "scene/theme/icons/file_thumbnail.svg",
    "scene/theme/icons/folder.svg", "scene/theme/icons/folder_create.svg", "scene/theme/icons/folder_thumbnail.svg",
    "scene/theme/icons/folder_up.svg", "scene/theme/icons/graph_port.svg", "scene/theme/icons/grid_layout.svg",
    "scene/theme/icons/grid_minimap.svg", "scene/theme/icons/grid_snap.svg", "scene/theme/icons/grid_toggle.svg",
    "scene/theme/icons/h_dragger.svg", "scene/theme/icons/hslider_tick.svg", "scene/theme/icons/hsplitter.svg",
    "scene/theme/icons/indeterminate.svg", "scene/theme/icons/indeterminate_disabled.svg", "scene/theme/icons/line_edit_clear.svg",
    "scene/theme/icons/load.svg", "scene/theme/icons/mini_checkerboard.svg", "scene/theme/icons/move_down.svg",
    "scene/theme/icons/move_up.svg", "scene/theme/icons/option_button_arrow.svg", "scene/theme/icons/picker_shape_circle.svg",
    "scene/theme/icons/picker_shape_rectangle.svg", "scene/theme/icons/picker_shape_rectangle_wheel.svg", "scene/theme/icons/popup_menu_arrow_left.svg",
    "scene/theme/icons/popup_menu_arrow_right.svg", "scene/theme/icons/radio_checked.svg", "scene/theme/icons/radio_checked_disabled.svg",
    "scene/theme/icons/radio_unchecked.svg", "scene/theme/icons/radio_unchecked_disabled.svg", "scene/theme/icons/region_folded.svg",
    "scene/theme/icons/region_unfolded.svg", "scene/theme/icons/reload.svg", "scene/theme/icons/resizer_nw.svg",
    "scene/theme/icons/resizer_se.svg", "scene/theme/icons/save.svg", "scene/theme/icons/script.svg",
    "scene/theme/icons/scroll_button_left.svg", "scene/theme/icons/scroll_button_left_hl.svg", "scene/theme/icons/scroll_button_right.svg",
    "scene/theme/icons/scroll_button_right_hl.svg", "scene/theme/icons/scroll_hint_horizontal.svg", "scene/theme/icons/scroll_hint_vertical.svg",
    "scene/theme/icons/search.svg", "scene/theme/icons/slider_grabber.svg", "scene/theme/icons/slider_grabber_disabled.svg",
    "scene/theme/icons/slider_grabber_hl.svg", "scene/theme/icons/sort.svg", "scene/theme/icons/tabs_drop_mark.svg",
    "scene/theme/icons/tabs_menu.svg", "scene/theme/icons/tabs_menu_hl.svg", "scene/theme/icons/text_edit_ellipsis.svg",
    "scene/theme/icons/text_edit_space.svg", "scene/theme/icons/text_edit_tab.svg", "scene/theme/icons/toggle_filename_filter.svg",
    "scene/theme/icons/toggle_off.svg", "scene/theme/icons/toggle_off_disabled.svg", "scene/theme/icons/toggle_off_disabled_mirrored.svg",
    "scene/theme/icons/toggle_off_mirrored.svg", "scene/theme/icons/toggle_on.svg", "scene/theme/icons/toggle_on_disabled.svg",
    "scene/theme/icons/toggle_on_disabled_mirrored.svg", "scene/theme/icons/toggle_on_mirrored.svg", "scene/theme/icons/unchecked.svg",
    "scene/theme/icons/unchecked_disabled.svg", "scene/theme/icons/updown.svg", "scene/theme/icons/v_dragger.svg",
    "scene/theme/icons/value_down.svg", "scene/theme/icons/value_up.svg", "scene/theme/icons/visibility_visible.svg",
    "scene/theme/icons/vslider_tick.svg", "scene/theme/icons/vsplitter.svg", "scene/theme/icons/zoom_less.svg",
    "scene/theme/icons/zoom_more.svg", "scene/theme/icons/zoom_reset.svg",
]

BUILTIN_FONTS = [
    "thirdparty/fonts/DroidSansFallback.woff2", "thirdparty/fonts/DroidSansJapanese.woff2",
    "thirdparty/fonts/Inter_Bold.woff2", "thirdparty/fonts/Inter_Regular.woff2",
    "thirdparty/fonts/JetBrainsMono_Regular.woff2", "thirdparty/fonts/NotoSansBengali_Bold.woff2",
    "thirdparty/fonts/NotoSansBengali_Regular.woff2", "thirdparty/fonts/NotoSansDevanagari_Bold.woff2",
    "thirdparty/fonts/NotoSansDevanagari_Regular.woff2", "thirdparty/fonts/NotoSansGeorgian_Bold.woff2",
    "thirdparty/fonts/NotoSansGeorgian_Regular.woff2", "thirdparty/fonts/NotoSansHebrew_Bold.woff2",
    "thirdparty/fonts/NotoSansHebrew_Regular.woff2", "thirdparty/fonts/NotoSansMalayalamUI_Bold.woff2",
    "thirdparty/fonts/NotoSansMalayalamUI_Regular.woff2", "thirdparty/fonts/NotoSansOriya_Bold.woff2",
    "thirdparty/fonts/NotoSansOriya_Regular.woff2", "thirdparty/fonts/NotoSansSinhala_Bold.woff2",
    "thirdparty/fonts/NotoSansSinhala_Regular.woff2", "thirdparty/fonts/NotoSansTamilUI_Bold.woff2",
    "thirdparty/fonts/NotoSansTamilUI_Regular.woff2", "thirdparty/fonts/NotoSansTeluguUI_Bold.woff2",
    "thirdparty/fonts/NotoSansTeluguUI_Regular.woff2", "thirdparty/fonts/NotoSansThai_Bold.woff2",
    "thirdparty/fonts/NotoSansThai_Regular.woff2", "thirdparty/fonts/Vazirmatn_Bold.woff2",
    "thirdparty/fonts/Vazirmatn_Regular.woff2",
]

DOC_CLASS_PATHS = {'EditorExportPlatformAndroid': 'platform/android/doc_classes', 'EditorExportPlatformIOS': 'platform/ios/doc_classes', 'EditorExportPlatformLinuxBSD': 'platform/linuxbsd/doc_classes', 'EditorExportPlatformMacOS': 'platform/macos/doc_classes', 'EditorExportPlatformVisionOS': 'platform/visionos/doc_classes', 'EditorExportPlatformWeb': 'platform/web/doc_classes', 'EditorExportPlatformWindows': 'platform/windows/doc_classes', 'CSGBox3D': 'modules/csg/doc_classes', 'CSGCombiner3D': 'modules/csg/doc_classes', 'CSGCylinder3D': 'modules/csg/doc_classes', 'CSGMesh3D': 'modules/csg/doc_classes', 'CSGPolygon3D': 'modules/csg/doc_classes', 'CSGPrimitive3D': 'modules/csg/doc_classes', 'CSGShape3D': 'modules/csg/doc_classes', 'CSGSphere3D': 'modules/csg/doc_classes', 'CSGTorus3D': 'modules/csg/doc_classes', 'ENetMultiplayerPeer': 'modules/enet/doc_classes', 'ENetConnection': 'modules/enet/doc_classes', 'ENetPacketPeer': 'modules/enet/doc_classes', 'EditorSceneFormatImporterFBX2GLTF': 'modules/fbx/doc_classes', 'EditorSceneFormatImporterUFBX': 'modules/fbx/doc_classes', 'FBXDocument': 'modules/fbx/doc_classes', 'FBXState': 'modules/fbx/doc_classes', 'EditorSceneFormatImporterBlend': 'modules/gltf/doc_classes', 'EditorSceneFormatImporterGLTF': 'modules/gltf/doc_classes', 'GLTFAccessor': 'modules/gltf/doc_classes', 'GLTFAnimation': 'modules/gltf/doc_classes', 'GLTFBufferView': 'modules/gltf/doc_classes', 'GLTFCamera': 'modules/gltf/doc_classes', 'GLTFDocument': 'modules/gltf/doc_classes', 'GLTFDocumentExtension': 'modules/gltf/doc_classes', 'GLTFDocumentExtensionConvertImporterMesh': 'modules/gltf/doc_classes', 'GLTFLight': 'modules/gltf/doc_classes', 'GLTFMesh': 'modules/gltf/doc_classes', 'GLTFNode': 'modules/gltf/doc_classes', 'GLTFObjectModelProperty': 'modules/gltf/doc_classes', 'GLTFPhysicsBody': 'modules/gltf/doc_classes', 'GLTFPhysicsShape': 'modules/gltf/doc_classes', 'GLTFSkeleton': 'modules/gltf/doc_classes', 'GLTFSkin': 'modules/gltf/doc_classes', 'GLTFSpecGloss': 'modules/gltf/doc_classes', 'GLTFState': 'modules/gltf/doc_classes', 'GLTFTexture': 'modules/gltf/doc_classes', 'GLTFTextureSampler': 'modules/gltf/doc_classes', 'GridMap': 'modules/gridmap/doc_classes', 'GridMapEditorPlugin': 'modules/gridmap/doc_classes', 'AudioStreamPlaylist': 'modules/interactive_music/doc_classes', 'AudioStreamPlaybackPlaylist': 'modules/interactive_music/doc_classes', 'AudioStreamInteractive': 'modules/interactive_music/doc_classes', 'AudioStreamPlaybackInteractive': 'modules/interactive_music/doc_classes', 'AudioStreamSynchronized': 'modules/interactive_music/doc_classes', 'AudioStreamPlaybackSynchronized': 'modules/interactive_music/doc_classes', 'MobileVRInterface': 'modules/mobile_vr/doc_classes', 'AudioStreamMP3': 'modules/mp3/doc_classes', 'ResourceImporterMP3': 'modules/mp3/doc_classes', 'SceneReplicationConfig': 'modules/multiplayer/doc_classes', 'SceneMultiplayer': 'modules/multiplayer/doc_classes', 'MultiplayerSpawner': 'modules/multiplayer/doc_classes', 'MultiplayerSynchronizer': 'modules/multiplayer/doc_classes', 'OfflineMultiplayerPeer': 'modules/multiplayer/doc_classes', 'FastNoiseLite': 'modules/noise/doc_classes', 'Noise': 'modules/noise/doc_classes', 'NoiseTexture2D': 'modules/noise/doc_classes', 'NoiseTexture3D': 'modules/noise/doc_classes', 'OggPacketSequence': 'modules/ogg/doc_classes', 'OggPacketSequencePlayback': 'modules/ogg/doc_classes', 'OpenXRInterface': 'modules/openxr/doc_classes', 'OpenXRAction': 'modules/openxr/doc_classes', 'OpenXRActionSet': 'modules/openxr/doc_classes', 'OpenXRActionMap': 'modules/openxr/doc_classes', 'OpenXRAPIExtension': 'modules/openxr/doc_classes', 'OpenXRExtensionWrapper': 'modules/openxr/doc_classes', 'OpenXRExtensionWrapperExtension': 'modules/openxr/doc_classes', 'OpenXRFrameSynthesisExtension': 'modules/openxr/doc_classes', 'OpenXRFutureResult': 'modules/openxr/doc_classes', 'OpenXRFutureExtension': 'modules/openxr/doc_classes', 'OpenXRInteractionProfile': 'modules/openxr/doc_classes', 'OpenXRInteractionProfileMetadata': 'modules/openxr/doc_classes', 'OpenXRIPBinding': 'modules/openxr/doc_classes', 'OpenXRHand': 'modules/openxr/doc_classes', 'OpenXRVisibilityMask': 'modules/openxr/doc_classes', 'OpenXRCompositionLayer': 'modules/openxr/doc_classes', 'OpenXRCompositionLayerQuad': 'modules/openxr/doc_classes', 'OpenXRCompositionLayerCylinder': 'modules/openxr/doc_classes', 'OpenXRCompositionLayerEquirect': 'modules/openxr/doc_classes', 'OpenXRBindingModifier': 'modules/openxr/doc_classes', 'OpenXRIPBindingModifier': 'modules/openxr/doc_classes', 'OpenXRActionBindingModifier': 'modules/openxr/doc_classes', 'OpenXRAnalogThresholdModifier': 'modules/openxr/doc_classes', 'OpenXRDpadBindingModifier': 'modules/openxr/doc_classes', 'OpenXRInteractionProfileEditorBase': 'modules/openxr/doc_classes', 'OpenXRInteractionProfileEditor': 'modules/openxr/doc_classes', 'OpenXRBindingModifierEditor': 'modules/openxr/doc_classes', 'OpenXRHapticBase': 'modules/openxr/doc_classes', 'OpenXRHapticVibration': 'modules/openxr/doc_classes', 'OpenXRRenderModelExtension': 'modules/openxr/doc_classes', 'OpenXRRenderModel': 'modules/openxr/doc_classes', 'OpenXRRenderModelManager': 'modules/openxr/doc_classes', 'OpenXRStructureBase': 'modules/openxr/doc_classes', 'OpenXRSpatialEntityExtension': 'modules/openxr/doc_classes', 'OpenXRSpatialEntityTracker': 'modules/openxr/doc_classes', 'OpenXRAnchorTracker': 'modules/openxr/doc_classes', 'OpenXRPlaneTracker': 'modules/openxr/doc_classes', 'OpenXRMarkerTracker': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationBaseHeader': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationAnchor': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationQrCode': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationMicroQrCode': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationAruco': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationAprilTag': 'modules/openxr/doc_classes', 'OpenXRSpatialContextPersistenceConfig': 'modules/openxr/doc_classes', 'OpenXRSpatialCapabilityConfigurationPlaneTracking': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentData': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentBounded2DList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentBounded3DList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentParentList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentMesh2DList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentMesh3DList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentPlaneAlignmentList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentPolygon2DList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentPlaneSemanticLabelList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentMarkerList': 'modules/openxr/doc_classes', 'OpenXRSpatialQueryResultData': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentAnchorList': 'modules/openxr/doc_classes', 'OpenXRSpatialComponentPersistenceList': 'modules/openxr/doc_classes', 'OpenXRSpatialAnchorCapability': 'modules/openxr/doc_classes', 'OpenXRSpatialPlaneTrackingCapability': 'modules/openxr/doc_classes', 'OpenXRSpatialMarkerTrackingCapability': 'modules/openxr/doc_classes', 'OpenXRAndroidThreadSettingsExtension': 'modules/openxr/doc_classes', 'OpenXRUserPresenceExtension': 'modules/openxr/doc_classes', 'RegEx': 'modules/regex/doc_classes', 'RegExMatch': 'modules/regex/doc_classes', 'TextServerAdvanced': 'modules/text_server_adv/doc_classes', 'VideoStreamTheora': 'modules/theora/doc_classes', 'UPNP': 'modules/upnp/doc_classes', 'UPNPDevice': 'modules/upnp/doc_classes', 'VisualShader': 'modules/visual_shader/doc_classes', 'VisualShaderNode': 'modules/visual_shader/doc_classes', 'VisualShaderNodeBillboard': 'modules/visual_shader/doc_classes', 'VisualShaderNodeBooleanConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeBooleanParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeClamp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeColorConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeColorFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeColorOp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeColorParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeComment': 'modules/visual_shader/doc_classes', 'VisualShaderNodeCompare': 'modules/visual_shader/doc_classes', 'VisualShaderNodeConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeCubemap': 'modules/visual_shader/doc_classes', 'VisualShaderNodeCubemapParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeCurveTexture': 'modules/visual_shader/doc_classes', 'VisualShaderNodeCurveXYZTexture': 'modules/visual_shader/doc_classes', 'VisualShaderNodeCustom': 'modules/visual_shader/doc_classes', 'VisualShaderNodeDerivativeFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeDeterminant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeDistanceFade': 'modules/visual_shader/doc_classes', 'VisualShaderNodeDotProduct': 'modules/visual_shader/doc_classes', 'VisualShaderNodeExpression': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFaceForward': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFloatConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFloatFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFloatOp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFloatParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFrame': 'modules/visual_shader/doc_classes', 'VisualShaderNodeFresnel': 'modules/visual_shader/doc_classes', 'VisualShaderNodeGlobalExpression': 'modules/visual_shader/doc_classes', 'VisualShaderNodeGroupBase': 'modules/visual_shader/doc_classes', 'VisualShaderNodeIf': 'modules/visual_shader/doc_classes', 'VisualShaderNodeInput': 'modules/visual_shader/doc_classes', 'VisualShaderNodeIntConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeIntFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeIntOp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeIntParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeIs': 'modules/visual_shader/doc_classes', 'VisualShaderNodeLinearSceneDepth': 'modules/visual_shader/doc_classes', 'VisualShaderNodeMix': 'modules/visual_shader/doc_classes', 'VisualShaderNodeMultiplyAdd': 'modules/visual_shader/doc_classes', 'VisualShaderNodeOuterProduct': 'modules/visual_shader/doc_classes', 'VisualShaderNodeOutput': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParameterRef': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleAccelerator': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleBoxEmitter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleConeVelocity': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleEmit': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleEmitter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleMeshEmitter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleMultiplyByAxisAngle': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleOutput': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleRandomness': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleRingEmitter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeParticleSphereEmitter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeProximityFade': 'modules/visual_shader/doc_classes', 'VisualShaderNodeRandomRange': 'modules/visual_shader/doc_classes', 'VisualShaderNodeRemap': 'modules/visual_shader/doc_classes', 'VisualShaderNodeReroute': 'modules/visual_shader/doc_classes', 'VisualShaderNodeResizableBase': 'modules/visual_shader/doc_classes', 'VisualShaderNodeRotationByAxis': 'modules/visual_shader/doc_classes', 'VisualShaderNodeSample3D': 'modules/visual_shader/doc_classes', 'VisualShaderNodeScreenNormalWorldSpace': 'modules/visual_shader/doc_classes', 'VisualShaderNodeScreenUVToSDF': 'modules/visual_shader/doc_classes', 'VisualShaderNodeSDFRaymarch': 'modules/visual_shader/doc_classes', 'VisualShaderNodeSDFToScreenUV': 'modules/visual_shader/doc_classes', 'VisualShaderNodeSmoothStep': 'modules/visual_shader/doc_classes', 'VisualShaderNodeStep': 'modules/visual_shader/doc_classes', 'VisualShaderNodeSwitch': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTexture': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTexture2DArray': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTexture2DArrayParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTexture2DParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTexture3D': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTexture3DParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTextureParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTextureParameterTriplanar': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTextureSDF': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTextureSDFNormal': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformCompose': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformDecompose': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformOp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeTransformVecMult': 'modules/visual_shader/doc_classes', 'VisualShaderNodeUIntConstant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeUIntFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeUIntOp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeUIntParameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeUVFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeUVPolarCoord': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVarying': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVaryingGetter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVaryingSetter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVec2Constant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVec2Parameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVec3Constant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVec3Parameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVec4Constant': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVec4Parameter': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorBase': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorCompose': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorDecompose': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorDistance': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorFunc': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorLen': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorOp': 'modules/visual_shader/doc_classes', 'VisualShaderNodeVectorRefract': 'modules/visual_shader/doc_classes', 'VisualShaderNodeWorldPositionFromDepth': 'modules/visual_shader/doc_classes', 'AudioStreamOggVorbis': 'modules/vorbis/doc_classes', 'AudioStreamPlaybackOggVorbis': 'modules/vorbis/doc_classes', 'ResourceImporterOggVorbis': 'modules/vorbis/doc_classes', 'WebRTCPeerConnection': 'modules/webrtc/doc_classes', 'WebRTCDataChannel': 'modules/webrtc/doc_classes', 'WebRTCMultiplayerPeer': 'modules/webrtc/doc_classes', 'WebRTCPeerConnectionExtension': 'modules/webrtc/doc_classes', 'WebRTCDataChannelExtension': 'modules/webrtc/doc_classes', 'WebSocketClient': 'modules/websocket/doc_classes', 'WebSocketMultiplayerPeer': 'modules/websocket/doc_classes', 'WebSocketPeer': 'modules/websocket/doc_classes', 'WebSocketServer': 'modules/websocket/doc_classes', 'WebXRInterface': 'modules/webxr/doc_classes', 'ZIPReader': 'modules/zip/doc_classes', 'ZIPPacker': 'modules/zip/doc_classes'}

EXTRACTABLE_SOURCES = [
    "editor/translations/extractable/ar.po", "editor/translations/extractable/bg.po",
    "editor/translations/extractable/ca.po", "editor/translations/extractable/cs.po",
    "editor/translations/extractable/de.po", "editor/translations/extractable/el.po",
    "editor/translations/extractable/es.po", "editor/translations/extractable/es_AR.po",
    "editor/translations/extractable/et.po", "editor/translations/extractable/extractable.pot",
    "editor/translations/extractable/fa.po", "editor/translations/extractable/fi.po",
    "editor/translations/extractable/fr.po", "editor/translations/extractable/he.po",
    "editor/translations/extractable/id.po", "editor/translations/extractable/it.po",
    "editor/translations/extractable/ja.po", "editor/translations/extractable/ko.po",
    "editor/translations/extractable/lv.po", "editor/translations/extractable/nl.po",
    "editor/translations/extractable/pl.po", "editor/translations/extractable/pt.po",
    "editor/translations/extractable/pt_BR.po", "editor/translations/extractable/ru.po",
    "editor/translations/extractable/sk.po", "editor/translations/extractable/sv.po",
    "editor/translations/extractable/th.po", "editor/translations/extractable/tr.po",
    "editor/translations/extractable/uk.po", "editor/translations/extractable/vi.po",
    "editor/translations/extractable/zh_Hans.po", "editor/translations/extractable/zh_Hant.po",
]


def _compile_extractable_entries(sources):
    valid_sources = [src for src in sources if os.path.isfile(src)]
    sorted_paths = sorted(
        [os.path.abspath(src) for src in valid_sources],
        key=lambda path: os.path.splitext(os.path.basename(path))[0],
    )
    xl_names = []
    blobs = []
    msgfmt = shutil.which("msgfmt")
    for path in sorted_paths:
        name = os.path.splitext(os.path.basename(path))[0]
        if msgfmt and name != "extractable":
            mo_path = os.path.join(tempfile.gettempdir(), uuid.uuid4().hex + ".mo")
            try:
                subprocess.run([msgfmt, path, "--no-hash", "-o", mo_path], stderr=subprocess.PIPE, check=False)
                buffer = methods.get_buffer(mo_path) if os.path.exists(mo_path) else methods.get_buffer(path)
            except OSError:
                buffer = methods.get_buffer(path)
            finally:
                if os.path.exists(mo_path):
                    try:
                        os.remove(mo_path)
                    except OSError:
                        pass
        else:
            buffer = methods.get_buffer(path)
            if name == "extractable":
                name = "source"
        decomp_size = len(buffer)
        buffer = methods.compress_buffer(buffer)
        blobs.append(
            f"inline constexpr const unsigned char _extractable_translation_{name}_compressed[] = {{\n"
            f"{methods.format_buffer(buffer, 1)}\n}};\n\n"
        )
        xl_names.append([name, len(buffer), decomp_size])
    return blobs, xl_names


def make_splash_and_icons(target, splash_src, icon_src):
    splash_buf = methods.get_buffer(str(splash_src))
    icon_buf = methods.get_buffer(str(icon_src))
    font_buf = methods.get_buffer("thirdparty/fonts/OpenSans_SemiBold.woff2")

    icons_names = []
    icons_raw = []
    for src in THEME_ICONS:
        with open(src, encoding="utf-8", newline="\n") as f:
            icons_raw.append(methods.to_raw_cstring(f.read()))
        icons_names.append(f'"{os.path.splitext(os.path.basename(src))[0]}"')

    icons_names_str = ",\n\t".join(icons_names)
    icons_raw_str = ",\n\t".join(icons_raw)

    doc_paths = dict(sorted((k, v) for k, v in DOC_CLASS_PATHS.items() if os.path.isdir(v)))
    doc_data = "\n".join([f'\t{{"{key}", "{value}"}},' for key, value in doc_paths.items()])

    platforms = ["android", "ios", "visionos", "web"]
    api_reg = "\n\t".join([f"register_{p}_api();" for p in platforms])
    api_unreg = "\n\t".join([f"unregister_{p}_api();" for p in platforms])

    modules = OrderedDict(methods.detect_modules("modules", recursive=False).items())
    decls = []
    for key in modules.keys():
        if key not in ("freetype", "msdfgen"):
            decls.append(f"void initialize_{key}_module(ModuleInitializationLevel p_level);")
            decls.append(f"void uninitialize_{key}_module(ModuleInitializationLevel p_level);")
    mod_inc = "\n".join(decls)
    mod_init = "\n".join([f"#ifdef MODULE_{key.upper()}_ENABLED\n\tinitialize_{key}_module(p_level);\n#endif" for key in modules.keys()])
    mod_uninit = "\n".join([f"#ifdef MODULE_{key.upper()}_ENABLED\n\tuninitialize_{key}_module(p_level);\n#endif" for key in modules.keys()])

    exp_platforms = ["android", "ios", "linuxbsd", "macos", "visionos", "web", "windows"]
    exp_reg = "\n\t".join([f"register_{p}_exporter();" for p in exp_platforms])
    exp_type = "\n\t".join([f"register_{p}_exporter_types();" for p in exp_platforms])
    ext_blobs, ext_xl_names = _compile_extractable_entries(EXTRACTABLE_SOURCES)

    with open(str(target), "w", encoding="utf-8", newline="\n") as out:
        out.write("/* THIS FILE IS GENERATED. EDITS WILL BE LOST. */\n\n")
        out.write("#ifndef MAIN_SPLASH_GEN_H\n#define MAIN_SPLASH_GEN_H\n\n")
        out.write(f"""\
#include <cstdint>
#include "core/math/vector2.h"

inline const Color boot_splash_bg_color = Color(0.14, 0.14, 0.14);
inline constexpr const unsigned char boot_splash_png[] = {{
{methods.format_buffer(splash_buf, 1)}
}};

inline constexpr const unsigned char app_icon_png[] = {{
{methods.format_buffer(icon_buf, 1)}
}};

#ifndef _FONT_OPENSANS_SEMIBOLD_DEFINED
#define _FONT_OPENSANS_SEMIBOLD_DEFINED
inline constexpr int _font_OpenSans_SemiBold_size = {len(font_buf)};
inline constexpr unsigned char _font_OpenSans_SemiBold[] = {{
	{methods.format_buffer(font_buf, 1)}
}};
#endif // _FONT_OPENSANS_SEMIBOLD_DEFINED

inline constexpr int default_theme_icons_count = {len(icons_names)};
inline constexpr const char *default_theme_icons_sources[] = {{
	{icons_raw_str}
}};

inline constexpr const char *default_theme_icons_names[] = {{
	{icons_names_str}
}};

""")
        for src in EXPORT_ICONS:
            src_path = Path(src)
            src_name = src_path.stem
            platform = src_path.parent.parent.stem
            with open(src, "r", encoding="utf-8") as f:
                svg = f.read()
            out.write(f"inline constexpr const char *_{platform}_{src_name}_svg = {methods.to_raw_cstring(svg)};\n")

        out.write(f"""
struct _DocDataClassPath {{
	const char *name;
	const char *path;
}};

inline constexpr int _doc_data_class_path_count = {len(doc_paths)};
inline constexpr _DocDataClassPath _doc_data_class_paths[{len(doc_paths) + 1}] = {{
{doc_data}
	{{nullptr, nullptr}},
}};

""")
        for fsrc in BUILTIN_FONTS:
            fbuf = methods.get_buffer(fsrc)
            fname = os.path.splitext(os.path.basename(fsrc))[0]
            out.write(
                f"inline constexpr int _font_{fname}_size = {len(fbuf)};\n"
                f"inline constexpr unsigned char _font_{fname}[] = {{\n\t{methods.format_buffer(fbuf, 1)}\n}};\n\n"
            )

        out.write("""\
#ifndef EDITOR_TRANSLATION_LIST
#define EDITOR_TRANSLATION_LIST

struct EditorTranslationList {
	const char* lang;
	int comp_size;
	int uncomp_size;
	const unsigned char* data;
};

#endif // EDITOR_TRANSLATION_LIST

extern const EditorTranslationList _extractable_translations[];
extern const EditorTranslationList _doc_translations[];

""")
        _write_license_parts(out, ["COPYRIGHT.txt", "LICENSE.txt"])
        out.write("\n#endif // MAIN_SPLASH_GEN_H\n\n")
        _write_shader_and_core_sections(out)
        out.write(f"""\
#ifdef DEFINE_MAIN_GENERATED_REGISTRIES
#ifndef MAIN_GENERATED_REGISTRIES_EMITTED
#define MAIN_GENERATED_REGISTRIES_EMITTED

#include "core/version_generated.gen.h"

void register_platform_apis() {{
	{api_reg}
}}

void unregister_platform_apis() {{
	{api_unreg}
}}

{mod_inc}

void initialize_modules(ModuleInitializationLevel p_level) {{
{mod_init}
}}

void uninitialize_modules(ModuleInitializationLevel p_level) {{
{mod_uninit}
}}

#endif // MAIN_GENERATED_REGISTRIES_EMITTED
#undef DEFINE_MAIN_GENERATED_REGISTRIES
#endif // DEFINE_MAIN_GENERATED_REGISTRIES

#ifdef DEFINE_EDITOR_EXPORTERS_AND_TRANSLATIONS
#ifndef EDITOR_EXPORTERS_AND_TRANSLATIONS_EMITTED
#define EDITOR_EXPORTERS_AND_TRANSLATIONS_EMITTED

void register_exporters() {{
	{exp_reg}
}}

void register_exporter_types() {{
	{exp_type}
}}

const EditorTranslationList _doc_translations[] = {{
	{{ nullptr, 0, 0, nullptr }},
}};

""")
        for b in ext_blobs:
            out.write(b)
        out.write("const EditorTranslationList _extractable_translations[] = {\n")
        for x in ext_xl_names:
            out.write(f'\t{{ "{x[0]}", {x[1]}, {x[2]}, _extractable_translation_{x[0]}_compressed }},\n')
        out.write(
            "\t{ nullptr, 0, 0, nullptr },\n};\n\n"
            "#endif // EDITOR_EXPORTERS_AND_TRANSLATIONS_EMITTED\n"
            "#undef DEFINE_EDITOR_EXPORTERS_AND_TRANSLATIONS\n"
            "#endif // DEFINE_EDITOR_EXPORTERS_AND_TRANSLATIONS\n"
        )


def _write_shader_and_core_sections(out):
    out.write("#ifdef DEFINE_RD_SHADERS_GEN\n")
    out.write("#ifndef RD_SHADERS_GEN_EMITTED\n#define RD_SHADERS_GEN_EMITTED\n\n")
    out.write('#include "core/io/ip.h"\n\n')
    for idx, shader in enumerate(SHADERS):
        if idx:
            out.write("\n")
        out.write(build_rd_shader_body(shader))
    for shader in RAW_SHADERS:
        out.write("\n")
        out.write(build_raw_shader_body(shader))
    out.write("\n#endif // RD_SHADERS_GEN_EMITTED\n#undef DEFINE_RD_SHADERS_GEN\n#endif // DEFINE_RD_SHADERS_GEN\n\n")

    out.write("#ifdef DEFINE_GLES3_SHADERS_GEN\n")
    out.write("#ifndef GLES3_SHADERS_GEN_EMITTED\n#define GLES3_SHADERS_GEN_EMITTED\n\n")
    for idx, shader in enumerate(GLES3_SHADERS):
        if idx:
            out.write("\n")
        _write_gles3_struct(out, shader)
    out.write("\n#endif // GLES3_SHADERS_GEN_EMITTED\n#undef DEFINE_GLES3_SHADERS_GEN\n#endif // DEFINE_GLES3_SHADERS_GEN\n\n")

    enc_hex = os.environ.get("SCRIPT_AES256_ENCRYPTION_KEY") or ("0" * 64)
    enc_buf = bytes.fromhex(enc_hex)
    if len(enc_buf) != 32:
        raise ValueError("Invalid SCRIPT_AES256_ENCRYPTION_KEY length")
    out.write("#ifdef DEFINE_CORE_GENERATED_GLOBALS\n")
    out.write("#ifndef CORE_GENERATED_GLOBALS_EMITTED\n#define CORE_GENERATED_GLOBALS_EMITTED\n\n")
    out.write('extern const char *const VLTR_VERSION_HASH;\n')
    out.write('extern const unsigned long long VLTR_VERSION_TIMESTAMP;\n')
    out.write('const char *const VLTR_VERSION_HASH = "9c2e8974959335fb84c88681cc92f90475684411";\n')
    out.write('const unsigned long long VLTR_VERSION_TIMESTAMP = 1785356176;\n\n')
    out.write(f"uint8_t script_encryption_key[32] = {{\n{methods.format_buffer(enc_buf, 1)}\n}};\n\n")
    _write_controller_mappings(
        out,
        ["core/input/gamecontrollerdb.txt", "core/input/godotcontrollerdb.txt"],
    )
    out.write("\n#endif // CORE_GENERATED_GLOBALS_EMITTED\n#undef DEFINE_CORE_GENERATED_GLOBALS\n#endif // DEFINE_CORE_GENERATED_GLOBALS\n\n")


def generate_version_and_modules():
    version_info_builder(
        ["core/version_generated.gen.h"],
        [
            {
                "short_name": "voltaire",
                "name": "Voltaire Engine",
                "major": 0,
                "minor": 0,
                "patch": 1,
                "status": "dev",
                "build": "custom_build",
                "module_config": "",
                "website": "blandlogic.com",
                "docs_branch": "latest",
            }
        ],
        methods.detect_modules("modules", recursive=False),
    )


def build_gles3_header(filename: str, shader: str, output_attribs: bool = True) -> None:
    with generated_wrapper(filename) as file:
        _write_gles3_struct(file, shader)


def build_raw_header(filename: str, shader: str) -> None:
    with generated_wrapper(filename) as file:
        file.write(build_raw_shader_body(shader))


def build_rd_header(filename: str, shader: str) -> None:
    with generated_wrapper(filename) as file:
        file.write(build_rd_shader_body(shader))


EDITOR_TRANSLATION_SOURCES = ["editor/translations/editor/ar.po", "editor/translations/editor/bg.po", "editor/translations/editor/bn.po", "editor/translations/editor/ca.po", "editor/translations/editor/cs.po", "editor/translations/editor/de.po", "editor/translations/editor/el.po", "editor/translations/editor/eo.po", "editor/translations/editor/es.po", "editor/translations/editor/es_AR.po", "editor/translations/editor/et.po", "editor/translations/editor/fa.po", "editor/translations/editor/fi.po", "editor/translations/editor/fr.po", "editor/translations/editor/ga.po", "editor/translations/editor/gl.po", "editor/translations/editor/he.po", "editor/translations/editor/hu.po", "editor/translations/editor/id.po", "editor/translations/editor/it.po", "editor/translations/editor/ja.po", "editor/translations/editor/ka.po", "editor/translations/editor/ko.po", "editor/translations/editor/nl.po", "editor/translations/editor/pl.po", "editor/translations/editor/pt.po", "editor/translations/editor/pt_BR.po", "editor/translations/editor/ro.po", "editor/translations/editor/ru.po", "editor/translations/editor/sk.po", "editor/translations/editor/sv.po", "editor/translations/editor/ta.po", "editor/translations/editor/th.po", "editor/translations/editor/tok.po", "editor/translations/editor/tr.po", "editor/translations/editor/uk.po", "editor/translations/editor/vi.po", "editor/translations/editor/zh_Hans.po", "editor/translations/editor/zh_Hant.po"]

PROPERTY_TRANSLATION_SOURCES = ["editor/translations/properties/bg.po", "editor/translations/properties/ca.po", "editor/translations/properties/cs.po", "editor/translations/properties/de.po", "editor/translations/properties/es.po", "editor/translations/properties/et.po", "editor/translations/properties/fa.po", "editor/translations/properties/fr.po", "editor/translations/properties/ga.po", "editor/translations/properties/hi.po", "editor/translations/properties/id.po", "editor/translations/properties/it.po", "editor/translations/properties/ja.po", "editor/translations/properties/ka.po", "editor/translations/properties/ko.po", "editor/translations/properties/pl.po", "editor/translations/properties/pt.po", "editor/translations/properties/pt_BR.po", "editor/translations/properties/ru.po", "editor/translations/properties/sv.po", "editor/translations/properties/ta.po", "editor/translations/properties/tr.po", "editor/translations/properties/uk.po", "editor/translations/properties/vi.po", "editor/translations/properties/zh_Hans.po", "editor/translations/properties/zh_Hant.po"]


def _compile_translation_entries(category, source):
    valid_sources = [src for src in source if os.path.isfile(src)]
    sorted_paths = sorted(
        [os.path.abspath(src) for src in valid_sources],
        key=lambda path: os.path.splitext(os.path.basename(path))[0],
    )
    xl_names = []
    blobs = []
    msgfmt = shutil.which("msgfmt")
    if not msgfmt:
        methods.print_warning("msgfmt not found, using .po files instead of .mo")

    for path in sorted_paths:
        name = os.path.splitext(os.path.basename(path))[0]
        if msgfmt and name != category:
            mo_path = os.path.join(tempfile.gettempdir(), uuid.uuid4().hex + ".mo")
            cmd = f'{msgfmt} "{path}" --no-hash -o "{mo_path}"'
            try:
                subprocess.Popen(cmd, shell=True, stderr=subprocess.PIPE).communicate()
                buffer = methods.get_buffer(mo_path)
            except OSError as e:
                methods.print_warning(
                    "msgfmt execution failed, using .po file instead of .mo: path=%r; [%s] %s"
                    % (path, e.__class__.__name__, e)
                )
                buffer = methods.get_buffer(path)
            finally:
                try:
                    if os.path.exists(mo_path):
                        os.remove(mo_path)
                except OSError as e:
                    methods.print_warning(
                        "Could not delete temporary .mo file: path=%r; [%s] %s"
                        % (mo_path, e.__class__.__name__, e)
                    )
        else:
            buffer = methods.get_buffer(path)
            if name == category:
                name = "source"

        decomp_size = len(buffer)
        buffer = methods.compress_buffer(buffer)
        blobs.append(
            f"inline constexpr const unsigned char _{category}_translation_{name}_compressed[] = {{\n"
            f"{methods.format_buffer(buffer, 1)}\n}};\n\n"
        )
        xl_names.append([name, len(buffer), decomp_size])

    return blobs, xl_names


def make_translations(target, source):
    target_h, target_cpp = str(target[0]), str(target[1])
    category = os.path.basename(target_cpp).split("_")[0]
    blobs, xl_names = _compile_translation_entries(category, source)

    with methods.generated_wrapper(target_cpp) as file:
        for b in blobs:
            file.write(b)
        file.write(f'#include "{target_h}"\n\nconst EditorTranslationList _{category}_translations[] = {{\n')
        for x in xl_names:
            file.write(f'\t{{ "{x[0]}", {x[1]}, {x[2]}, _{category}_translation_{x[0]}_compressed }},\n')
        file.write("\t{ nullptr, 0, 0, nullptr },\n};\n")

    with methods.generated_wrapper(target_h) as file:
        file.write(f"""
#ifndef EDITOR_TRANSLATION_LIST
#define EDITOR_TRANSLATION_LIST

struct EditorTranslationList {{
\tconst char* lang;
\tint comp_size;
\tint uncomp_size;
\tconst unsigned char* data;
}};

#endif // EDITOR_TRANSLATION_LIST

extern const EditorTranslationList _{category}_translations[];
""")


def generate_translations(idx=None):
    if idx in (None, 0):
        make_translations(
            ["editor/translations/editor_translations.gen.h", "editor/translations/editor_translations.gen.cpp"],
            EDITOR_TRANSLATION_SOURCES,
        )
    if idx in (None, 1):
        make_translations(
            ["editor/translations/property_translations.gen.h", "editor/translations/property_translations.gen.cpp"],
            PROPERTY_TRANSLATION_SOURCES,
        )


def generate_all():
    generate_version_and_modules()
    make_splash_and_icons("main/splash.gen.h", "main/splash.png", "main/app_icon.png")
    make_icu_data(["thirdparty/icu4c/icudata.gen.h"], ["thirdparty/icu4c/icudt_godot.dat"])
    generate_translations()
    generate_wayland_protocols(sowrap=True)


REQUIRED_GENERATED_FILES = [
    "core/version_generated.gen.h",
    "main/splash.gen.h",
    "thirdparty/icu4c/icudata.gen.h",
    "editor/translations/editor_translations.gen.h",
    "editor/translations/editor_translations.gen.cpp",
    "editor/translations/property_translations.gen.h",
    "editor/translations/property_translations.gen.cpp",
]

WAYLAND_GENERATED_FILES = [
    "platform/linuxbsd/wayland/protocol/wayland_protocols.gen.h",
    "platform/linuxbsd/wayland/protocol/wayland_protocols.gen.c",
]


def _missing_generated_files():
    req = list(REQUIRED_GENERATED_FILES)
    if shutil.which("wayland-scanner"):
        req.extend(WAYLAND_GENERATED_FILES)
    return [p for p in req if not os.path.exists(p)]


if __name__ == "__main__":
    if "--if-needed" in sys.argv:
        missing = _missing_generated_files()
        if not missing:
            sys.exit(0)
        if set(missing) <= set(WAYLAND_GENERATED_FILES):
            generate_wayland_protocols(sowrap=True)
            sys.exit(0)
    if "--splash-only" in sys.argv:
        make_splash_and_icons("main/splash.gen.h", "main/splash.png", "main/app_icon.png")
        sys.exit(0)
    if "--version-only" in sys.argv:
        generate_version_and_modules()
        sys.exit(0)
    if "--translations-only" in sys.argv:
        arg_idx = sys.argv.index("--translations-only")
        which = int(sys.argv[arg_idx + 1]) if arg_idx + 1 < len(sys.argv) else None
        generate_translations(which)
        sys.exit(0)
    generate_all()
