#!/usr/bin/env python3

import re
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

LINKER_EXCLUSIONS = {
    "bin/obj/thirdparty/harfbuzz/src/hb-subset.linuxbsd.editor.x86_64.o",
    "bin/obj/thirdparty/harfbuzz/src/hb-vector-paint.linuxbsd.editor.x86_64.o",
    "bin/obj/thirdparty/harfbuzz/src/hb-vector-paint-svg.linuxbsd.editor.x86_64.o",
}

ARCHIVE_COMMAND_PATTERN = re.compile(r"(\b\w+\.a\b)+")


def simplify_linker_command(cmd: str, rsp_path="bin/objects.rsp"):
    cmd = re.sub(r"bin/obj/.*\.o", f"@{rsp_path}", cmd)
    cmd = re.sub(r"\s+bin/obj/.*\.a", "", cmd)
    return re.sub(r" +", " ", cmd).strip()


def filter_object_files(object_files):
    filtered = []
    for obj in object_files:
        if obj in LINKER_EXCLUSIONS:
            continue
        filtered.append(obj)
    icu_ushape = "bin/obj/thirdparty/icu4c/common/ushape.linuxbsd.editor.x86_64.o"
    filtered.append(icu_ushape)
    return sorted(filtered)


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
    HEADER_EXTS = (".h", ".hpp", ".hh", ".hxx", ".H")
    all_files = set(changed_paths.split())
    headers_to_trace = {f for f in all_files if f.endswith(HEADER_EXTS)}
    while headers_to_trace:
        trace_cmd = f"./tools/trace-includes {' '.join(headers_to_trace)}"
        changed_files = subprocess.run(
            shlex.split(trace_cmd), check=True, capture_output=True, text=True
        )
        output_paths = changed_files.stdout.strip()
        if not output_paths:
            break
        discovered_files = set(output_paths.split())
        # Identify files not yet seen in previous iterations
        new_files = discovered_files - all_files
        all_files.update(new_files)
        # Prepare only the newly discovered headers for the next pass
        headers_to_trace = {f for f in new_files if f.endswith(HEADER_EXTS)}
    result = list(all_files)
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


def is_linker_command(line: str):
    """Check if a line contains static archive files (.a), identifying it as a linker step."""
    return bool(ARCHIVE_COMMAND_PATTERN.search(line))


def generate_comp_file(name, obj_list_name):
    cont = input(
        "Warning: this will delete any existing build artifacts. Continue? (y/N)\n"
    )
    if "y" not in cont.lower():
        print("File generation aborted.")
        return
    print("Generating compilation file...")
    subprocess.run(shlex.split("scons -c"), check=True)
    scons_cmd = "scons -n progress=no verbose=yes CPPDEFINES=VOLTAIRE_BUILD"
    comp_file = subprocess.run(
        shlex.split(scons_cmd),
        check=True,
        capture_output=True,
        text=True,
    )
    lines = comp_file.stdout.split("\n")
    comp_commands = []
    object_files = []
    obj_pattern = re.compile(r"-o\s+([^\s]+|\"[^\"]+\")")
    # the first 7 lines are scons boilerplate
    for line in lines[6:]:
        line_str = line.strip()
        if not line_str or line_str.startswith(("ar ", "ranlib ", "scons:", "INFO:")):
            continue
        comp_commands.append(line_str)
        match = obj_pattern.search(line_str)
        if match:
            obj_path = match.group(1).strip("\"'")
            if obj_path.endswith(".o"):
                object_files.append(obj_path)
    object_files = filter_object_files(object_files)
    print("Writing compilation commands...")
    with open(name, "w") as file:
        for line in comp_commands:
            if is_linker_command(line):
                line = simplify_linker_command(line)
            file.write(f"{line}\n")
    print("Writing response file...")
    with open(obj_list_name, "w") as file:
        for obj in object_files:
            file.write(f"{obj}\n")
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
                print(f"{line}\n")
                try_exec(cmd, shell)
        # if nothing executed by here, search for python funtion
        if not found:
            if line:
                cmd = construct_python_command(line)
                print(f"{line}\n")
                try_exec(get_shell_command(cmd), False)


# TODO: make platform agnostic
def link_object_files():
    subprocess.run(
        shlex.split(
            "g++ -o bin/voltaire.linuxbsd.editor.x86_64 -static-libgcc -static-libstdc++ -s -O2 @bin/objects.txt -Lbin/build_deps/accesskit/lib/linux/x86_64/static -laccesskit -lrt -lpthread -ldl"
        ),
        check=True,
    )
