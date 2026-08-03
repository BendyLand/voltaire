#!/usr/bin/env python3

import shlex
import subprocess
import sys

DUPS = {
    "build_gles3_headers": 0,
    "build_raw_headers": 0,
    "build_rd_headers": 0,
    "export_icon_builder": 0,
    "make_fonts_header": 0,
    "make_translations": 0,
    "run": 0,
}


def get_run_arg():
    run = ""
    if len(sys.argv) > 1:
        run = sys.argv[1]
        try:
            run = int(run)
            return run
        except ValueError as e:
            print(f"Invalid argument: {e}")
            sys.exit(1)
    else:
        print("Usage: scripts/`this`.py <N>")
        sys.exit(1)


def try_exec(cmd, shell):
    try:
        subprocess.run(cmd, check=True, shell=shell)
    except subprocess.CalledProcessError as e:
        print(f"Error: {e}")


def get_shell_command(string):
    if "wayland-scanner" not in string:
        return shlex.split(string)
    else:
        return string


def get_python_command(string):
    if "(" in string:
        return string[: string.index("(")]
    else:
        return string


def construct_python_command(string):
    cmd = get_python_command(string)
    if cmd in DUPS:
        n = DUPS[cmd]
        DUPS[cmd] += 1
        return f"scripts/{cmd}.py {n}"
    else:
        return f"scripts/{cmd}.py"


def read_file(path):
    lines = []
    with open(path) as file:
        for line in file:
            lines.append(line)
    return lines


def write_file(path, contents):
    with open(path, "w") as file:
        for line in contents:
            file.write(f"{line}\n")


def remove_headers(files):
    HEADER_EXTS = (".h", ".hpp", ".hh", ".hxx", ".H")
    return [f for f in files if not f.endswith(HEADER_EXTS)]


def keep_source_files(files):
    FILE_EXTS = (".c", ".cpp", ".cc", ".cxx", ".C")
    return [f for f in files if f.endswith(FILE_EXTS)]


def get_changed_files():
    print("Locating changed files...")
    changes = subprocess.run(
        shlex.split("./tools/watcher ."), check=True, capture_output=True, text=True
    )
    changed_paths = changes.stdout.strip()
    if not changed_paths:
        print("No changed files detected.")
        return []
    result = changed_paths.split(" ")
    trace_cmd = f"./tools/trace-includes {changed_paths}"
    changed_files = subprocess.run(
        shlex.split(trace_cmd), check=True, capture_output=True, text=True
    )
    result.extend(changed_files.stdout.strip().split(" "))
    result = remove_headers(result)
    result = keep_source_files(result)
    print(f"Done! Compiling {len(result)} files.")
    return result


def find_cmd_for_path(path, comp):
    for line in comp:
        if path in line:
            return line
    return ""


def clean_file_path(path):
    while path[0] == "." or path[0] == "/":
        path = path[1:]
    return path


def get_compilation_cmds(changed_files, comp):
    result = []
    for line in comp:
        if not line.startswith("g++") and not line.startswith("gcc"):
            result.append(line.strip())
    for file in changed_files:
        cmd = find_cmd_for_path(clean_file_path(file), comp)
        result.append(cmd.strip())
    return result


def generate_comp_file(name):
    # FIX: add the -DVOLTAIRE_BUILD flag to core/crypto/crypto_core.cpp and servers/rendering/rendering_device.cpp
    cont = input("Warning: this will delete any existing build artifacts. Continue? (y/N)\n")
    if "y" not in cont.lower():
        print("File generation aborted.")
        return
    print("Generating compilation file...")
    subprocess.run(shlex.split("scons -c"), check=True)
    comp_file = subprocess.run(
        shlex.split("scons -n progress=no verbose=yes"),
        check=True,
        capture_output=True,
        text=True,
    )
    lines = comp_file.stdout.split("\n")
    result = []
    # the first 7 lines are scons boilerplate
    for line in lines[6:]:
        if line.startswith("ar ") or line.startswith("ranlib ") or line.startswith("scons:") or line.startswith("INFO:"):
            continue
        result.append(line)
    with open(name, "w") as file:
        for line in result:
            file.write(f"{line}\n")
    print("Done!")


options = {"gcc", "g++", "wayland-scanner"}
lines = read_file("comp")


def run_full_build():
    for line in lines:
        found = False
        for option in options:
            if line.startswith(option):
                found = True
                shell = False
                if "wayland-scanner" in line:
                    shell = True
                    cmd = line
                else:
                    cmd = get_shell_command(line)
                print(line)
                try_exec(cmd, shell)
        # if nothing executed by here, search for python funtion
        if not found:
            if line:
                cmd = construct_python_command(line)
                print(line)
                try_exec(get_shell_command(cmd), False)


def run_incremental_build():
    changed_files = get_changed_files()
    compilation_commands = get_compilation_cmds(changed_files, lines)
    for line in compilation_commands:
        found = False
        for option in options:
            if line.startswith(option):
                found = True
                shell = False
                if "wayland-scanner" in line:
                    shell = True
                    cmd = line
                else:
                    cmd = get_shell_command(line)
                print(line)
                try_exec(cmd, shell)
        # if nothing executed by here, search for python funtion
        if not found:
            if line:
                cmd = construct_python_command(line)
                print(line)
                try_exec(get_shell_command(cmd), False)
