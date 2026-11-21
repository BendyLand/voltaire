import os
import platform
import shutil
import subprocess
import sys

import methods

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
