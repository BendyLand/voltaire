from __future__ import annotations

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
from platform_methods import detect_arch

base_folder = Path(__file__).resolve().parent

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

    if not recursive:
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
