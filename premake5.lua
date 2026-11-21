-- =============================================================================
-- Premake5 Cross-Platform Build Script for Voltaire Engine
--
-- Replaces SCons (`SConstruct`, `platform/*/detect.py`, and all `SCsub` files)
-- while preserving upstream Godot's full target, platform, architecture, and
-- feature matrix for Make (`gmake2` / `gmake`), Ninja (`ninja`), Visual Studio
-- (`vs2022`), and Clangd (`compilecommands`).
--
-- Targets (configurations):
--   editor            Full Voltaire Editor (TOOLS_ENABLED + DEBUG_ENABLED, -O2)
--   template_debug    Export template with debugging/profiling (DEBUG_ENABLED, -O2)
--   template_release  Production export template (-O3, stripped, no debug/tools)
--
-- Target Platforms (`--p=<platform>`):
--   linuxbsd          Linux / *BSD (x86_64, x86_32, arm64, arm32, rv64, ppc64, loongarch64)
--   windows           Windows via MinGW-w64 or MSVC (x86_64, x86_32, arm64, arm32)
--   macos             macOS native, OSXCross, or Apple LLVM cross (arm64, x86_64)
--   android           Android NDK SharedLib libgodot_android.so (arm64, arm32, x86_64, x86_32)
--   ios               iOS device & simulator StaticLib libgodot.a (arm64, x86_64)
--   visionos          visionOS device & simulator StaticLib libgodot.a (arm64, x86_64)
--   web               WebAssembly via Emscripten emcc/em++ (wasm32, wasm64)
--
-- Common Usage Examples:
--   premake5 gmake2                         && make -C build -j$(nproc)
--   premake5 ninja                          && ninja -C build
--   premake5 ninja                          && ninja -C build template_release_x86_64
--   premake5 --p=windows --arch=x86_64 ninja && ninja -C build
--   premake5 --p=macos --arch=arm64 ninja   && ninja -C build
--   premake5 --p=android --arch=arm64 ninja && ninja -C build
--   premake5 --p=ios --arch=arm64 ninja     && ninja -C build
--   premake5 --p=web --arch=wasm32 ninja    && ninja -C build
--   premake5 --help                         (list all Voltaire build options)
-- =============================================================================

-- =============================================================================
-- 1. COMMAND-LINE OPTIONS (parity with SConstruct & platform/*/detect.py)
-- =============================================================================

newoption {
    trigger = "p",
    value = "PLATFORM",
    description = "Target platform (auto-detected from host OS if omitted)",
    default = "auto",
    allowed = {
        { "auto",     "Auto-detect host OS (linuxbsd, windows, or macos)" },
        { "linuxbsd", "Linux / *BSD desktop (X11 / Wayland)" },
        { "linux",    "Alias for linuxbsd" },
        { "bsd",      "Alias for linuxbsd" },
        { "windows",  "Windows desktop (MinGW-w64 or MSVC)" },
        { "macos",    "macOS desktop (Apple Clang / OSXCross)" },
        { "osx",      "Alias for macos" },
        { "android",  "Android mobile/XR (Android NDK LLVM)" },
        { "ios",      "iOS mobile (Xcode / OSXCross)" },
        { "visionos", "Apple visionOS spatial (Xcode / OSXCross)" },
        { "web",      "Web / HTML5 WebAssembly (Emscripten)" },
    }
}

newoption {
    trigger = "arch",
    value = "ARCH",
    description = "Target CPU architecture ('auto' for platform default, 'all' for all valid arches)",
    default = "auto",
    allowed = {
        { "auto",        "Auto-detect default architecture for target platform" },
        { "all",         "Generate all architectures supported by target platform" },
        { "x86_64",      "64-bit x86 (SSE4.2 + POPCNT)" },
        { "x86_32",      "32-bit x86 (SSE2)" },
        { "arm64",       "64-bit ARM (AArch64 / NEON)" },
        { "arm32",       "32-bit ARM (ARMv7 / NEON)" },
        { "rv64",        "64-bit RISC-V (rv64gc)" },
        { "ppc64",       "64-bit PowerPC" },
        { "loongarch64", "64-bit LoongArch (LSX)" },
        { "wasm32",      "32-bit WebAssembly" },
        { "wasm64",      "64-bit WebAssembly (MEMORY64)" },
    }
}

local function bool_option(trigger, default_val, desc)
    newoption {
        trigger = trigger,
        value = "yes|no",
        default = default_val,
        description = desc,
        allowed = {
            { "yes", "Enabled" },
            { "no",  "Disabled" },
        }
    }
end

local function tri_option(trigger, desc)
    newoption {
        trigger = trigger,
        value = "auto|yes|no",
        default = "auto",
        description = desc,
        allowed = {
            { "auto", "Platform default" },
            { "yes",  "Force enabled" },
            { "no",   "Force disabled" },
        }
    }
end

-- Core compilation options
bool_option("dev-build", "no", "Developer build with DEV_ENABLED assertions and unoptimized debug code")
tri_option("debug-symbols", "Emit DWARF/PDB debug symbols (auto: yes for editor/dev-build, no for templates)")

newoption {
    trigger = "optimize",
    value = "LEVEL",
    default = "auto",
    description = "Optimization level override",
    allowed = {
        { "auto",        "Inferred from target (speed_trace for editor/template_debug, speed for template_release, size for web)" },
        { "none",        "No optimization (-O0 / /Od)" },
        { "debug",       "Debug-friendly optimization (-Og)" },
        { "speed_trace", "Fast execution with traceable stackframes (-O2)" },
        { "speed",       "Maximum execution speed (-O3 / /O2)" },
        { "size",        "Optimize for binary size (-Os / /O1)" },
        { "size_extra",  "Optimize aggressively for size (-Os + SIZE_EXTRA)" },
    }
}

newoption {
    trigger = "precision",
    value = "TYPE",
    default = "single",
    description = "Floating-point precision for real_t / Jolt Physics",
    allowed = {
        { "single", "32-bit single-precision float (default)" },
        { "double", "64-bit double-precision float (REAL_T_IS_DOUBLE)" },
    }
}

newoption {
    trigger = "lto",
    value = "MODE",
    default = "none",
    description = "Link-time optimization mode",
    allowed = {
        { "none", "Disable LTO (fastest link)" },
        { "auto", "Platform-recommended production LTO" },
        { "thin", "LLVM ThinLTO (-flto=thin)" },
        { "full", "Full LTO (-flto / /GL + /LTCG)" },
    }
}

bool_option("production", "no", "Enable production defaults (static C++ runtime, LTO auto)")
bool_option("threads", "yes", "Enable multithreading support (THREADS_ENABLED)")
bool_option("deprecated", "yes", "Keep compatibility code for deprecated APIs")
bool_option("strict-checks", "no", "Enable STRICT_CHECKS debug invariants")
bool_option("brotli", "yes", "Enable built-in Brotli decoder (WOFF2 / compression)")
bool_option("graphite", "yes", "Enable SIL Graphite2 smart font shaping")
bool_option("mp3-extra-formats", "no", "Enable MP1/MP2 decoding in minimp3")

newoption {
    trigger = "warnings",
    value = "LEVEL",
    default = "no",
    description = "Compiler warning verbosity on engine sources",
    allowed = {
        { "no",       "Disable warnings (-w / /w)" },
        { "moderate", "Moderate warnings (-Wall -Wno-unused / /W2)" },
        { "all",      "Standard Godot warnings (-Wall / /W3)" },
        { "extra",    "Strict warnings (-Wall -Wextra / /W4)" },
    }
}
bool_option("werror", "no", "Treat compiler/linker warnings as errors")

newoption {
    trigger = "library-type",
    value = "TYPE",
    default = "executable",
    description = "Output binary type (executable or LibGodot static/shared library)",
    allowed = {
        { "executable",     "Standard engine executable / platform app binary" },
        { "static_library", "LibGodot static library (LIBGODOT_ENABLED)" },
        { "shared_library", "LibGodot shared library (LIBGODOT_ENABLED)" },
    }
}

-- Toolchain & sanitizer options
bool_option("use-llvm", "no", "Use Clang / clang-cl instead of GCC / MSVC")
bool_option("use-static-cpp", "yes", "Link libgcc/libstdc++ or MSVC CRT statically")

newoption {
    trigger = "linker",
    value = "LINKER",
    default = "default",
    description = "Linker program for GCC/Clang (-fuse-ld=...)",
    allowed = {
        { "default", "System default linker" },
        { "bfd",     "GNU ld.bfd" },
        { "gold",    "GNU ld.gold" },
        { "lld",     "LLVM lld" },
        { "mold",    "mold fast linker" },
    }
}

bool_option("asan", "no", "Enable AddressSanitizer (ASAN)")
bool_option("ubsan", "no", "Enable UndefinedBehaviorSanitizer (UBSAN)")
bool_option("tsan", "no", "Enable ThreadSanitizer (TSAN)")
bool_option("lsan", "no", "Enable LeakSanitizer (LSAN)")

-- Rendering, audio, input & accessibility drivers
tri_option("vulkan", "Enable Vulkan rendering driver")
tri_option("use-volk", "Load Vulkan dynamically via volk")
tri_option("opengl3", "Enable OpenGL 3.3 / OpenGL ES 3.0 Compatibility renderer")
tri_option("d3d12", "Enable Direct3D 12 rendering driver (Windows)")
tri_option("metal", "Enable Metal rendering driver (Apple arm64)")
tri_option("angle", "Enable ANGLE OpenGL ES 3.0 translation layer (Windows/macOS)")
tri_option("sdl", "Enable SDL3 gamepad/haptic input driver")
tri_option("accesskit", "Enable AccessKit screen-reader accessibility driver")
bool_option("xaudio2", "no", "Enable XAudio2 audio driver (Windows)")

newoption {
    trigger = "accesskit-sdk-path",
    value = "PATH",
    default = "",
    description = "Custom path to AccessKit C SDK (defaults to bin/build_deps/accesskit)"
}

-- Linux / *BSD options
bool_option("sowrap", "yes", "Dynamically load Linux/BSD system libraries via *-so_wrap")
bool_option("x11", "yes", "Enable X11 display server backend (linuxbsd)")
bool_option("wayland", "yes", "Enable Wayland display server backend (linuxbsd)")
bool_option("libdecor", "yes", "Enable libdecor client-side decorations on Wayland")
bool_option("alsa", "yes", "Enable ALSA audio/MIDI driver (linuxbsd)")
bool_option("pulseaudio", "yes", "Enable PulseAudio driver (linuxbsd)")
bool_option("dbus", "yes", "Enable D-Bus portal/screensaver integration (linuxbsd)")
bool_option("speechd", "yes", "Enable Speech Dispatcher TTS support (linuxbsd)")
bool_option("fontconfig", "yes", "Enable Fontconfig system font discovery (linuxbsd)")
bool_option("udev", "yes", "Enable libudev gamepad hotplugging (linuxbsd)")
bool_option("execinfo", "no", "Link libexecinfo for crash handler on non-glibc (musl/*BSD)")

-- Windows options
tri_option("use-mingw", "Use MinGW-w64 instead of MSVC on Windows (auto: yes when cross-compiling or gmake/ninja without cl)")
newoption { trigger = "mingw-prefix", value = "PATH", default = "", description = "MinGW installation prefix (or $MINGW_PREFIX)" }
newoption {
    trigger = "windows-subsystem",
    value = "MODE",
    default = "gui",
    description = "Windows executable subsystem (gui also builds voltaire_console wrapper)",
    allowed = {
        { "gui",     "WINDOWS subsystem + companion console wrapper executable" },
        { "console", "CONSOLE subsystem (single executable)" },
    }
}
tri_option("winrt", "Enable WinRT OneCore TTS support on Windows (C++20)")
bool_option("debug-crt", "no", "Use MSVC dynamic debug CRT (/MDd)")
newoption { trigger = "mesa-libs", value = "PATH", default = "", description = "Path to Godot Mesa/NIR static libraries for D3D12" }
newoption { trigger = "angle-libs", value = "PATH", default = "", description = "Path to ANGLE static libraries" }
newoption { trigger = "agility-sdk-path", value = "PATH", default = "", description = "Path to DirectX 12 Agility SDK" }
newoption { trigger = "pix-path", value = "PATH", default = "", description = "Path to WinPixEventRuntime SDK" }
newoption { trigger = "winrt-path", value = "PATH", default = "", description = "Path to cppwinrt headers for MinGW" }

-- Apple (macOS / iOS / visionOS) options
newoption { trigger = "apple-sdk-path", value = "PATH", default = "", description = "Explicit macOS/iOS/visionOS SDK sysroot path" }
newoption { trigger = "apple-toolchain-path", value = "PATH", default = "", description = "Explicit Xcode toolchain path (for iOS/visionOS)" }
newoption { trigger = "apple-target-triple", value = "TRIPLE", default = "", description = "Apple cross-toolchain binary prefix" }
newoption { trigger = "osxcross-sdk", value = "SDK", default = "darwin16", description = "OSXCross SDK suffix (e.g. darwin16)" }
newoption { trigger = "vulkan-sdk-path", value = "PATH", default = "", description = "Path to MoltenVK / Vulkan SDK on macOS/iOS" }
bool_option("simulator", "no", "Target iOS / visionOS Simulator instead of physical device")

-- Android options
newoption { trigger = "android-home", value = "PATH", default = "", description = "Path to Android SDK ($ANDROID_HOME / $ANDROID_SDK_ROOT)" }
newoption { trigger = "ndk-platform", value = "API", default = "android-24", description = "Minimum Android API level (e.g. android-24)" }
tri_option("swappy", "Link Android Swappy frame-pacing library if present in thirdparty/swappy-frame-pacing")

-- Web / Emscripten options
bool_option("wasm-simd", "yes", "Enable WebAssembly 128-bit SIMD (-msimd128)")
bool_option("javascript-eval", "yes", "Enable JavaScriptBridge.eval() interface (JAVASCRIPT_EVAL_ENABLED)")
bool_option("dlink", "no", "Enable WebAssembly dynamic linking / GDExtension side-module support")
bool_option("proxy-to-pthread", "no", "Run main loop on a pthread worker (-sPROXY_TO_PTHREAD=1)")
bool_option("closure-compiler", "no", "Minify Emscripten JS glue with Closure Compiler")
newoption { trigger = "initial-memory", value = "MB", default = "32", description = "Initial WebAssembly heap memory in MiB (min 64 for editor)" }
newoption { trigger = "stack-size", value = "KB", default = "5120", description = "WebAssembly stack size in KiB" }

-- Export-template feature stripping options
bool_option("disable-3d", "no", "Disable 3D nodes, physics, navigation, XR, and 3D modules (templates only)")
bool_option("disable-advanced-gui", "no", "Disable advanced GUI controls (templates only)")
bool_option("disable-physics-2d", "no", "Disable 2D physics server & nodes (templates only)")
bool_option("disable-physics-3d", "no", "Disable 3D physics server & Jolt/VHACD (templates only)")
bool_option("disable-navigation-2d", "no", "Disable 2D navigation server & RVO2-2D (templates only)")
bool_option("disable-navigation-3d", "no", "Disable 3D navigation server, Recast & RVO2-3D (templates only)")
bool_option("disable-xr", "no", "Disable XR server, OpenXR, MobileVR & WebXR")
bool_option("betsy-export-templates", "no", "Include Betsy GPU texture compressor in export templates")
bool_option("cvtt-export-templates", "no", "Include CVTT texture compressor in export templates")

-- =============================================================================
-- 2. PLATFORM, ARCHITECTURE & OPTION RESOLUTION
-- =============================================================================

local function opt_bool(name)
    return _OPTIONS[name] == "yes"
end

local function opt_tri(name, default_bool)
    local v = _OPTIONS[name]
    if v == "yes" then return true end
    if v == "no" then return false end
    return default_bool
end

local host_os = os.host() -- "linux", "windows", "macosx", "bsd", ...

local target_platform = _OPTIONS["p"] or "auto"
if target_platform == "auto" then
    if host_os == "windows" then
        target_platform = "windows"
    elseif host_os == "macosx" then
        target_platform = "macos"
    else
        target_platform = "linuxbsd"
    end
elseif target_platform == "linux" or target_platform == "bsd" then
    target_platform = "linuxbsd"
elseif target_platform == "osx" then
    target_platform = "macos"
end

local supported_arches_by_platform = {
    linuxbsd = { "x86_64", "x86_32", "arm64", "arm32", "rv64", "ppc64", "loongarch64" },
    windows  = { "x86_64", "x86_32", "arm64", "arm32" },
    macos    = { "arm64", "x86_64" },
    android  = { "arm64", "arm32", "x86_64", "x86_32" },
    ios      = { "arm64", "x86_64" },
    visionos = { "arm64", "x86_64" },
    web      = { "wasm32", "wasm64" },
}

local function detect_default_arch(plat)
    if plat == "android" or plat == "ios" or plat == "visionos" then
        return "arm64"
    elseif plat == "web" then
        return "wasm32"
    end
    local ha = os.hostarch and os.hostarch() or "x86_64"
    local map = {
        x86_64  = "x86_64",
        amd64   = "x86_64",
        x86     = "x86_32",
        i386    = "x86_32",
        i686    = "x86_32",
        ARM64   = "arm64",
        arm64   = "arm64",
        aarch64 = "arm64",
        ARM     = "arm32",
        arm     = "arm32",
        riscv64 = "rv64",
        ppc64   = "ppc64",
        ppc64le = "ppc64",
        loongarch64 = "loongarch64",
    }
    local detected = map[ha] or "x86_64"
    for _, a in ipairs(supported_arches_by_platform[plat]) do
        if a == detected then return detected end
    end
    return supported_arches_by_platform[plat][1]
end

local arch_opt = _OPTIONS["arch"] or "auto"
local active_arches = {}
if arch_opt == "all" then
    active_arches = supported_arches_by_platform[target_platform]
elseif arch_opt == "auto" then
    active_arches = { detect_default_arch(target_platform) }
else
    local valid = false
    for _, a in ipairs(supported_arches_by_platform[target_platform]) do
        if a == arch_opt then valid = true; break end
    end
    if not valid then
        error(string.format(
            "Architecture '%s' is not supported on platform '%s'. Supported: %s",
            arch_opt, target_platform, table.concat(supported_arches_by_platform[target_platform], ", ")
        ))
    end
    active_arches = { arch_opt }
end
local primary_arch = active_arches[1]

local is_simulator = opt_bool("simulator")
if target_platform == "ios" and primary_arch == "x86_64" and not is_simulator then
    is_simulator = true
end

-- Determine MSVC vs MinGW on Windows
local action_name = _ACTION or ""
local is_vs_action = action_name:sub(1, 2) == "vs"
local is_msvc = false
if target_platform == "windows" then
    local default_mingw = (host_os ~= "windows") or (not is_vs_action and _OPTIONS["cc"] == "gcc")
    local use_mingw = opt_tri("use-mingw", default_mingw)
    is_msvc = not use_mingw
end

local is_production = opt_bool("production")
local is_dev_build = opt_bool("dev-build")
local use_llvm = opt_bool("use-llvm")
if target_platform == "macos" or target_platform == "ios" or target_platform == "visionos" or target_platform == "android" or target_platform == "web" then
    use_llvm = true
end

local use_static_cpp = opt_bool("use-static-cpp")
local lto_mode = _OPTIONS["lto"] or "none"
if is_production and lto_mode == "none" then
    lto_mode = "auto"
end
if lto_mode == "auto" then
    if target_platform == "linuxbsd" or (target_platform == "windows" and not is_msvc) then
        lto_mode = use_llvm and "thin" or "full"
    elseif target_platform == "web" then
        lto_mode = "thin"
    else
        lto_mode = "none"
    end
end

-- Dependency directory discovery (matches platform/*/detect.py)
local deps_folder = "bin/build_deps"
local localappdata = os.getenv("LOCALAPPDATA")
if host_os == "windows" and localappdata and not os.getenv("MSYSTEM") and os.isdir(localappdata .. "/Godot/build_deps") then
    deps_folder = localappdata:gsub("\\", "/") .. "/Godot/build_deps"
end

local accesskit_sdk_path = _OPTIONS["accesskit-sdk-path"]
if not accesskit_sdk_path or accesskit_sdk_path == "" then
    accesskit_sdk_path = deps_folder .. "/accesskit"
end

local mesa_libs_base = _OPTIONS["mesa-libs"]
if not mesa_libs_base or mesa_libs_base == "" then
    mesa_libs_base = deps_folder .. "/mesa"
end

local angle_libs_base = _OPTIONS["angle-libs"]
if not angle_libs_base or angle_libs_base == "" then
    angle_libs_base = deps_folder .. "/angle"
end

local winrt_mingw_path = _OPTIONS["winrt-path"]
if not winrt_mingw_path or winrt_mingw_path == "" then
    winrt_mingw_path = deps_folder .. "/winrt_mingw"
end

local agility_sdk_path = _OPTIONS["agility-sdk-path"]
if not agility_sdk_path or agility_sdk_path == "" then
    agility_sdk_path = deps_folder .. "/agility_sdk"
end

local pix_path = _OPTIONS["pix-path"]
if not pix_path or pix_path == "" then
    pix_path = deps_folder .. "/pix"
end

-- Driver resolution (matching platform/*/detect.py defaults)
local default_vulkan = (target_platform ~= "visionos" and target_platform ~= "web") and not (is_simulator and (target_platform == "ios" or target_platform == "visionos"))
local use_vulkan = opt_tri("vulkan", default_vulkan)
if target_platform == "visionos" or target_platform == "web" then
    use_vulkan = false
end

local default_volk = (target_platform == "linuxbsd" or target_platform == "windows" or target_platform == "android")
local use_volk = use_vulkan and opt_tri("use-volk", default_volk)

local default_opengl3 = (target_platform ~= "visionos")
local use_opengl3 = opt_tri("opengl3", default_opengl3)
if target_platform == "visionos" then
    use_opengl3 = false
end

local function has_mesa_sdk()
    if os.isdir(mesa_libs_base) then return true end
    for _, a in ipairs(active_arches) do
        local suf = is_msvc and "-msvc" or (use_llvm and "-llvm" or "-gcc")
        if os.isdir(mesa_libs_base .. "-" .. a .. suf) then return true end
    end
    return false
end

local use_d3d12 = (target_platform == "windows") and opt_tri("d3d12", has_mesa_sdk())
if use_d3d12 and not has_mesa_sdk() and _OPTIONS["d3d12"] == "yes" then
    print("WARNING: --d3d12=yes requested, but Mesa/NIR SDK not found at " .. mesa_libs_base .. " (run python3 misc/scripts/install_d3d12_sdk_windows.py)")
end

local default_metal = (target_platform == "macos" or target_platform == "ios" or target_platform == "visionos") and not is_simulator and (primary_arch == "arm64" or arch_opt == "all")
local use_metal = opt_tri("metal", default_metal)
if target_platform ~= "macos" and target_platform ~= "ios" and target_platform ~= "visionos" then
    use_metal = false
end

local use_rd = use_vulkan or use_d3d12 or use_metal

local function resolve_angle_dir(arch)
    if target_platform == "windows" then
        local suf = is_msvc and "-msvc" or (use_llvm and "-llvm" or "-gcc")
        local p = angle_libs_base .. "-" .. arch .. suf
        if os.isdir(p) then return p end
    elseif target_platform == "macos" then
        local p = angle_libs_base .. "-" .. arch .. "-macos"
        if os.isdir(p) then return p end
    end
    if os.isdir(angle_libs_base) then return angle_libs_base end
    return nil
end

local use_angle = use_opengl3 and (target_platform == "windows" or target_platform == "macos") and opt_tri("angle", resolve_angle_dir(primary_arch) ~= nil)
local use_sdl = (target_platform == "linuxbsd" or target_platform == "windows" or target_platform == "macos" or target_platform == "ios" or target_platform == "visionos") and opt_tri("sdl", true)
local use_accesskit = (target_platform == "linuxbsd" or target_platform == "windows" or target_platform == "macos") and opt_tri("accesskit", os.isdir(accesskit_sdk_path))
local use_xaudio2 = (target_platform == "windows") and opt_bool("xaudio2")
local use_brotli = opt_bool("brotli")
local use_graphite = opt_bool("graphite")
local use_threads = opt_bool("threads")
local library_type = _OPTIONS["library-type"] or "executable"

-- Feature-disable flags (export templates)
local disable_3d = opt_bool("disable-3d")
local disable_adv_gui = opt_bool("disable-advanced-gui")
local disable_phys_2d = opt_bool("disable-physics-2d")
local disable_phys_3d = disable_3d or opt_bool("disable-physics-3d")
local disable_nav_2d = opt_bool("disable-navigation-2d")
local disable_nav_3d = disable_3d or opt_bool("disable-navigation-3d")
local disable_xr = disable_3d or opt_bool("disable-xr")
local betsy_templates = opt_bool("betsy-export-templates")
local cvtt_templates = opt_bool("cvtt-export-templates")

-- Module availability
local has_openxr = (not disable_xr) and (target_platform == "linuxbsd" or target_platform == "windows" or target_platform == "macos" or target_platform == "android")
local has_camera = (target_platform == "linuxbsd" or target_platform == "windows" or target_platform == "macos" or target_platform == "android" or target_platform == "ios" or target_platform == "visionos") and (host_os ~= "bsd")
local has_webxr = (not disable_xr) and use_opengl3 and not (target_platform == "web" and opt_bool("proxy-to-pthread"))
local has_theora = (primary_arch ~= "rv64") or (arch_opt == "all")

local function arch_supports_raycast(arch)
    if disable_3d or target_platform == "web" then return false end
    if arch == "arm64" and target_platform == "windows" and is_msvc then return false end
    if arch == "x86_64" or arch == "arm64" then return true end
    if arch == "x86_32" and target_platform == "windows" then return true end
    return false
end

local has_raycast_any = false
for _, a in ipairs(active_arches) do
    if arch_supports_raycast(a) then has_raycast_any = true end
end

local has_pcre2_jit = (target_platform ~= "ios" and target_platform ~= "visionos" and target_platform ~= "web")

-- Windows WinRT check
local default_winrt = is_msvc or os.isdir(winrt_mingw_path)
local use_winrt = (target_platform == "windows") and opt_tri("winrt", default_winrt)

-- Cross-toolchain binary overrides for Premake's gcc/clang toolsets (works in both gmake/gmake2 and ninja!)
local android_ndk_root = ""
local android_api = tonumber((_OPTIONS["ndk-platform"] or "android-24"):match("(%d+)")) or 24
if android_api < 24 then android_api = 24 end

if target_platform == "android" then
    local sdk_root = _OPTIONS["android-home"]
    if not sdk_root or sdk_root == "" then
        sdk_root = os.getenv("ANDROID_HOME") or os.getenv("ANDROID_SDK_ROOT") or ""
    end
    android_ndk_root = os.getenv("ANDROID_NDK_ROOT") or (sdk_root ~= "" and (sdk_root .. "/ndk/29.0.14206865") or "")
    local host_sub = "linux-x86_64"
    if host_os == "macosx" then host_sub = "darwin-x86_64"
    elseif host_os == "windows" then host_sub = "windows-x86_64" end
    if android_ndk_root ~= "" and premake and premake.tools and premake.tools.clang then
        premake.tools.clang.tools.ar = "llvm-ar"
    end
    android_ndk_BinPrefix = android_ndk_root ~= "" and (android_ndk_root:gsub("\\", "/") .. "/toolchains/llvm/prebuilt/" .. host_sub .. "/bin/") or ""
elseif target_platform == "web" then
    if premake and premake.tools and premake.tools.clang then
        premake.tools.clang.tools.cc = "emcc"
        premake.tools.clang.tools.cxx = "em++"
        premake.tools.clang.tools.ar = "emar"
    end
end

-- Apple SDK discovery helper
local apple_sdk_path = _OPTIONS["apple-sdk-path"] or ""
if apple_sdk_path == "" then
    if target_platform == "macos" then
        apple_sdk_path = os.getenv("MACOS_SDK_PATH") or ""
    elseif target_platform == "ios" then
        apple_sdk_path = os.getenv("IOS_SDK_PATH") or os.getenv("APPLE_SDK_PATH") or ""
    elseif target_platform == "visionos" then
        apple_sdk_path = os.getenv("VISIONOS_SDK_PATH") or os.getenv("APPLE_SDK_PATH") or ""
    end
    if apple_sdk_path == "" and host_os == "macosx" and os.outputof then
        local sdk_name = "macosx"
        if target_platform == "ios" then
            sdk_name = is_simulator and "iphonesimulator" or "iphoneos"
        elseif target_platform == "visionos" then
            sdk_name = is_simulator and "xrsimulator" or "xros"
        end
        local detected_sdk = os.outputof("xcrun --sdk " .. sdk_name .. " --show-sdk-path 2>/dev/null")
        if detected_sdk then
            apple_sdk_path = detected_sdk:gsub("%s+$", "")
        end
    end
end

local apple_toolchain_path = _OPTIONS["apple-toolchain-path"] or os.getenv("APPLE_TOOLCHAIN_PATH") or os.getenv("IOS_TOOLCHAIN_PATH") or ""
if apple_toolchain_path == "" and host_os == "macosx" and (target_platform == "ios" or target_platform == "visionos") then
    apple_toolchain_path = "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain"
end

-- Configurations list (first entry is the default Make/Ninja target!)
local target_configs
if target_platform == "ios" or target_platform == "visionos" then
    target_configs = { "template_debug", "template_release" }
elseif target_platform == "android" or target_platform == "web" then
    target_configs = { "template_debug", "template_release", "editor" }
else
    target_configs = { "editor", "template_debug", "template_release" }
end

-- =============================================================================
-- 3. WORKSPACE DEFINITION
-- =============================================================================

workspace "voltaire"
    location "build"
    configurations(target_configs)
    platforms(active_arches)
    defaultplatform(primary_arch)
    startproject "voltaire"

    -- Target OS semantics (ensures cross-builds never inherit host /usr/lib64 or wrong extensions)
    if target_platform == "windows" then
        system "windows"
    elseif target_platform == "macos" then
        system "macosx"
    elseif target_platform == "ios" or target_platform == "visionos" then
        system "ios"
    elseif target_platform == "android" then
        system "android"
    elseif host_os == "bsd" then
        system "bsd"
    elseif target_platform == "linuxbsd" then
        system "linux"
    end

    -- Toolset selection
    if is_msvc then
        if use_llvm then
            toolset "clang"
        else
            toolset "msc"
        end
    elseif use_llvm then
        toolset "clang"
    else
        toolset "gcc"
    end

    -- Architecture mapping (only set x86_64/x86/ARM64/ARM where Premake supports them cleanly;
    -- omit on wasm/rv64/ppc64/loongarch64 so Premake never injects -m64/-m32)
    filter "platforms:x86_64"
        architecture "x86_64"
    filter "platforms:x86_32"
        architecture "x86"
    filter "platforms:arm64"
        architecture "ARM64"
    filter "platforms:arm32"
        architecture "ARM"
    filter {}

    -- Cross-compiler prefixes
    if target_platform == "windows" and not is_msvc then
        local mingw_root = _OPTIONS["mingw-prefix"]
        if not mingw_root or mingw_root == "" then
            mingw_root = os.getenv("MINGW_PREFIX") or ""
        end
        local bin_dir = (mingw_root ~= "") and (mingw_root:gsub("\\", "/") .. "/bin/") or ""
        if host_os ~= "windows" or mingw_root ~= "" then
            filter "platforms:x86_64"
                gccprefix(bin_dir .. "x86_64-w64-mingw32-")
            filter "platforms:x86_32"
                gccprefix(bin_dir .. "i686-w64-mingw32-")
            filter "platforms:arm64"
                gccprefix(bin_dir .. "aarch64-w64-mingw32-")
            filter "platforms:arm32"
                gccprefix(bin_dir .. "armv7-w64-mingw32-")
            filter {}
        end
    elseif target_platform == "android" and android_ndk_BinPrefix ~= "" then
        gccprefix(android_ndk_BinPrefix)
    elseif target_platform == "macos" and os.getenv("OSXCROSS_ROOT") then
        local osx_root = os.getenv("OSXCROSS_ROOT"):gsub("\\", "/")
        local osx_sdk = _OPTIONS["osxcross-sdk"] or "darwin16"
        filter "platforms:arm64"
            gccprefix(osx_root .. "/target/bin/arm64-apple-" .. osx_sdk .. "-")
        filter "platforms:x86_64"
            gccprefix(osx_root .. "/target/bin/x86_64-apple-" .. osx_sdk .. "-")
        filter {}
    elseif (target_platform == "ios" or target_platform == "visionos") and apple_toolchain_path ~= "" then
        local triple = _OPTIONS["apple-target-triple"] or ""
        gccprefix(apple_toolchain_path:gsub("\\", "/") .. "/usr/bin/" .. triple)
    end

-- =============================================================================
-- 4. COMMON COMPILER, TARGET & PLATFORM SETTINGS
-- =============================================================================

local function apply_common_settings(opts)
    opts = opts or {}
    language "C++"

    if is_msvc then
        cdialect "C17"
        cppdialect(opts.cpp20 and "C++20" or "C++17")
    else
        cdialect "gnu17"
        cppdialect(opts.cpp20 and "gnu++20" or "gnu++17")
    end

    if opts.exceptions then
        exceptionhandling "On"
    else
        exceptionhandling "Off"
        if is_msvc then
            defines { "_HAS_EXCEPTIONS=0" }
        end
    end

    if target_platform == "android" or library_type == "shared_library" then
        pic "On"
    else
        pic "Off"
    end

    local out_subdir = (target_platform == "linuxbsd" and host_os == "linux")
        and "bin/%{cfg.platform}/%{cfg.buildcfg}"
        or ("bin/" .. target_platform .. "/%{cfg.platform}/%{cfg.buildcfg}")
    targetdir(out_subdir)
    objdir("build/obj/" .. target_platform .. "/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")

    includedirs { "." }

    -- Universal engine defines
    defines {
        "VOLTAIRE_BUILD",
        "MINIZIP_ENABLED",
        "CLIPPER2_ENABLED",
        "ZSTD_STATIC_LINKING_ONLY",
        "ZSTD_DISABLE_ASM",
        "OVERRIDE_ENABLED",
    }

    if use_brotli then
        defines { "BROTLI_ENABLED" }
    end
    if use_threads then
        defines { "THREADS_ENABLED" }
    end
    if not is_msvc then
        defines { "_LIBCPP_REMOVE_TRANSITIVE_INCLUDES" }
    end
    if _OPTIONS["precision"] == "double" then
        defines { "REAL_T_IS_DOUBLE" }
    end
    if not opt_bool("deprecated") then
        defines { "DISABLE_DEPRECATED" }
    end
    if opt_bool("strict-checks") then
        defines { "STRICT_CHECKS" }
    end
    if library_type ~= "executable" then
        defines { "LIBGODOT_ENABLED" }
    end
    if is_dev_build then
        defines { "DEV_ENABLED", "ZLIB_DEBUG" }
    else
        defines { "NDEBUG" }
    end

    -- Active renderer & input driver defines
    if use_vulkan then
        defines { "VULKAN_ENABLED" }
    end
    if use_d3d12 then
        defines { "D3D12_ENABLED", "DCOMP_ENABLED" }
    end
    if use_rd then
        defines { "RD_ENABLED" }
    end
    if use_opengl3 then
        defines { "GLES3_ENABLED" }
    end
    if use_sdl then
        defines { "SDL_ENABLED" }
    end

    -- Target-specific defines (editor vs template_debug vs template_release)
    filter "configurations:editor"
        defines {
            "TOOLS_ENABLED",
            "DEBUG_ENABLED",
            "ENGINE_UPDATE_CHECK_ENABLED",
            "NO_EDITOR_SPLASH",
            "OVERRIDE_PATH_ENABLED",
        }

    filter "configurations:template_debug"
        defines {
            "DEBUG_ENABLED",
            "MODULE_LIGHTMAPPER_RD_DISABLED",
            "MODULE_XATLAS_UNWRAP_DISABLED",
        }
        if not betsy_templates then
            defines { "MODULE_BETSY_DISABLED" }
        end
        if not cvtt_templates then
            defines { "MODULE_CVTT_DISABLED" }
        end

    filter "configurations:template_release"
        defines {
            "MODULE_LIGHTMAPPER_RD_DISABLED",
            "MODULE_XATLAS_UNWRAP_DISABLED",
        }
        if not betsy_templates then
            defines { "MODULE_BETSY_DISABLED" }
        end
        if not cvtt_templates then
            defines { "MODULE_CVTT_DISABLED" }
        end

    filter "configurations:template_debug or template_release"
        if disable_3d then
            defines {
                "_3D_DISABLED",
                "MODULE_CSG_DISABLED",
                "MODULE_FBX_DISABLED",
                "MODULE_GLTF_DISABLED",
                "MODULE_GRIDMAP_DISABLED",
                "MODULE_MESHOPTIMIZER_DISABLED",
            }
        end
        if disable_adv_gui then
            defines { "ADVANCED_GUI_DISABLED" }
        end
        if disable_phys_2d then
            defines { "PHYSICS_2D_DISABLED", "MODULE_GODOT_PHYSICS_2D_DISABLED" }
        end
        if disable_phys_3d then
            defines { "PHYSICS_3D_DISABLED", "MODULE_JOLT_PHYSICS_DISABLED", "MODULE_VHACD_DISABLED" }
        end
        if disable_nav_2d then
            defines { "NAVIGATION_2D_DISABLED", "MODULE_NAVIGATION_2D_DISABLED" }
        end
        if disable_nav_3d then
            defines { "NAVIGATION_3D_DISABLED", "MODULE_NAVIGATION_3D_DISABLED" }
        end
        if disable_xr then
            defines { "XR_DISABLED", "MODULE_MOBILE_VR_DISABLED", "MODULE_OPENXR_DISABLED", "MODULE_WEBXR_DISABLED" }
        end
    filter {}

    -- Platform/arch module disable guards (syncs with modules/modules_enabled.gen.h)
    if not use_rd then
        defines { "MODULE_GLSLANG_DISABLED" }
    end
    if not has_openxr then
        defines { "MODULE_OPENXR_DISABLED" }
    end
    if not has_camera then
        defines { "MODULE_CAMERA_DISABLED" }
    end
    if not has_webxr then
        defines { "MODULE_WEBXR_DISABLED" }
    end
    for _, a in ipairs(active_arches) do
        if not arch_supports_raycast(a) then
            filter("platforms:" .. a)
                defines { "MODULE_RAYCAST_DISABLED" }
            filter {}
        end
    end
    filter "platforms:rv64"
        defines { "MODULE_THEORA_DISABLED" }
    filter {}

    -- -------------------------------------------------------------------------
    -- Platform-specific compiler flags, includes & defines
    -- -------------------------------------------------------------------------
    if target_platform == "linuxbsd" then
        includedirs { "platform/linuxbsd" }
        defines {
            "LINUXBSD_ENABLED",
            "UNIX_ENABLED",
            "_FILE_OFFSET_BITS=64",
        }
        if opt_bool("sowrap") then
            includedirs { "thirdparty/linuxbsd_headers" }
            defines { "SOWRAP_ENABLED", "XKB_ENABLED" }
            if opt_bool("wayland") then
                includedirs { "thirdparty/linuxbsd_headers/wayland" }
                if opt_bool("libdecor") then
                    includedirs { "thirdparty/linuxbsd_headers/libdecor-0" }
                end
            end
        end
        if opt_bool("x11") then defines { "X11_ENABLED" } end
        if opt_bool("wayland") then
            defines { "WAYLAND_ENABLED" }
            if opt_bool("libdecor") then defines { "LIBDECOR_ENABLED" } end
        end
        if opt_bool("alsa") then defines { "ALSA_ENABLED", "ALSAMIDI_ENABLED" } end
        if opt_bool("pulseaudio") then defines { "PULSEAUDIO_ENABLED", "_REENTRANT" } end
        if opt_bool("dbus") and use_threads then defines { "DBUS_ENABLED" } end
        if opt_bool("speechd") then defines { "SPEECHD_ENABLED" } end
        if opt_bool("fontconfig") then defines { "FONTCONFIG_ENABLED" } end
        if host_os == "linux" and opt_bool("udev") then defines { "UDEV_ENABLED" } end
        defines { "TOUCH_ENABLED", "CRASH_HANDLER_ENABLED" }
        if use_accesskit then
            includedirs { accesskit_sdk_path .. "/include" }
            defines { "ACCESSKIT_ENABLED" }
        end

    elseif target_platform == "windows" then
        includedirs { "platform/windows" }
        defines {
            "WINDOWS_ENABLED",
            "WASAPI_ENABLED",
            "WINMIDI_ENABLED",
            "WINVER=0x0A00",
            "_WIN32_WINNT=0x0A00",
            "NOMINMAX",
            "ENABLE_PREFER_HIGH_PERFORMANCE_GPU",
        }
        if _OPTIONS["windows-subsystem"] == "console" then
            defines { "WINDOWS_SUBSYSTEM_CONSOLE" }
        end
        if use_xaudio2 then
            defines { "XAUDIO2_ENABLED" }
        end
        if use_accesskit then
            includedirs { accesskit_sdk_path .. "/include" }
            defines { "ACCESSKIT_ENABLED" }
        end
        if use_angle then
            includedirs { "thirdparty/angle/include" }
            defines { "ANGLE_ENABLED", "EGL_STATIC" }
        end
        if is_msvc then
            defines { "TYPED_METHOD_BIND", "WIN32" }
            filter "platforms:x86_64"
                defines { "_WIN64" }
            filter {}
            buildoptions { "/fp:strict", "/Gd", "/GR", "/utf-8", "/bigobj", "/permissive-", "/Zc:__cplusplus" }
            if opt_bool("debug-crt") then
                staticruntime "Off"
                runtime "Debug"
            else
                staticruntime(use_static_cpp and "On" or "Off")
                runtime "Release"
            end
        else
            defines { "MINGW_ENABLED", "MINGW_HAS_SECURE_API=1" }
            buildoptions { "-Wa,-mbig-obj" }
            if not use_llvm then
                buildoptions { "-mwindows" }
            end
        end

    elseif target_platform == "macos" then
        includedirs { "platform/macos" }
        defines {
            "MACOS_ENABLED",
            "UNIX_ENABLED",
            "COREAUDIO_ENABLED",
            "COREMIDI_ENABLED",
        }
        buildoptions { "-fobjc-arc", "-fvisibility=hidden" }
        if apple_sdk_path ~= "" then
            buildoptions { "-isysroot", apple_sdk_path }
        end
        if os.getenv("APPLE_LLVM_CROSS") then
            filter "platforms:arm64"
                buildoptions { "-target", "arm64-apple-darwin" }
            filter "platforms:x86_64"
                buildoptions { "-target", "x86_64-apple-darwin" }
            filter {}
        end
        filter "platforms:arm64"
            buildoptions { "-arch", "arm64", "-mmacosx-version-min=13.0" }
            if use_metal then
                includedirs { "thirdparty/spirv-cross", "thirdparty/metal-cpp" }
                defines { "METAL_ENABLED", "RD_ENABLED" }
            end
        filter "platforms:x86_64"
            buildoptions { "-arch", "x86_64", "-mmacosx-version-min=11.0" }
        filter {}
        if use_accesskit then
            includedirs { accesskit_sdk_path .. "/include" }
            defines { "ACCESSKIT_ENABLED" }
        end
        if use_angle then
            includedirs { "thirdparty/angle/include" }
            defines { "ANGLE_ENABLED", "EGL_STATIC" }
        end

    elseif target_platform == "android" then
        includedirs { "platform/android" }
        defines {
            "ANDROID_ENABLED",
            "UNIX_ENABLED",
            "_FILE_OFFSET_BITS=64",
        }
        buildoptions {
            "-ffunction-sections",
            "-funwind-tables",
            "-fstack-protector-strong",
            "-fvisibility=hidden",
        }
        filter "platforms:arm64"
            buildoptions { "-target", "aarch64-linux-android" .. tostring(android_api), "-mfix-cortex-a53-835769" }
            defines { "__ARM_ARCH_8A__" }
        filter "platforms:arm32"
            buildoptions { "-target", "armv7a-linux-androideabi" .. tostring(android_api), "-march=armv7-a", "-mfloat-abi=softfp" }
            defines { "__ARM_ARCH_7__", "__ARM_ARCH_7A__", "__ARM_NEON__" }
        filter "platforms:x86_64"
            buildoptions { "-target", "x86_64-linux-android" .. tostring(android_api) }
        filter "platforms:x86_32"
            buildoptions { "-target", "i686-linux-android" .. tostring(android_api) }
        filter {}

    elseif target_platform == "ios" or target_platform == "visionos" then
        local plat_dir = (target_platform == "ios") and "platform/ios" or "platform/visionos"
        includedirs { plat_dir, "thirdparty/metal-cpp" }
        defines {
            (target_platform == "ios") and "IOS_ENABLED" or "VISIONOS_ENABLED",
            "APPLE_EMBEDDED_ENABLED",
            "UNIX_ENABLED",
            "COREAUDIO_ENABLED",
        }
        buildoptions {
            "-fobjc-arc",
            "-fblocks",
            "-fpascal-strings",
            "-fvisibility=hidden",
            "-Wno-ambiguous-macro",
        }
        if apple_sdk_path ~= "" then
            buildoptions { "-isysroot", apple_sdk_path }
            sysincludedirs {
                apple_sdk_path .. "/usr/include",
                apple_sdk_path .. "/System/Library/Frameworks/AudioUnit.framework/Headers",
            }
            if use_metal then
                sysincludedirs {
                    apple_sdk_path .. "/System/Library/Frameworks/Metal.framework/Headers",
                    apple_sdk_path .. "/System/Library/Frameworks/MetalFX.framework/Headers",
                    apple_sdk_path .. "/System/Library/Frameworks/QuartzCore.framework/Headers",
                }
            end
            if use_opengl3 and target_platform == "ios" then
                sysincludedirs {
                    apple_sdk_path .. "/System/Library/Frameworks/OpenGLES.framework/Headers",
                }
            end
        end
        if target_platform == "ios" then
            if is_simulator then
                defines { "IOS_SIMULATOR" }
                buildoptions { "-mios-simulator-version-min=15.0" }
            else
                buildoptions { "-miphoneos-version-min=15.0" }
            end
            if use_opengl3 then
                defines { "GLES_SILENCE_DEPRECATION" }
                buildoptions { "-Wno-module-import-in-extern-c" }
            end
        else
            if is_simulator then
                defines { "VISIONOS_SIMULATOR" }
                buildoptions { "-mtargetos=xros26.0-simulator" }
            else
                buildoptions { "-mtargetos=xros26.0" }
            end
        end
        filter "platforms:arm64"
            buildoptions { "-arch", "arm64" }
            if use_metal then
                includedirs { "thirdparty/spirv-cross" }
                defines { "METAL_ENABLED", "RD_ENABLED" }
            end
        filter "platforms:x86_64"
            buildoptions { "-arch", "x86_64" }
        filter {}

    elseif target_platform == "web" then
        includedirs { "platform/web" }
        defines {
            "WEB_ENABLED",
            "UNIX_ENABLED",
            "UNIX_SOCKET_UNAVAILABLE",
            "GDSCRIPT_NO_LSP",
        }
        if opt_bool("javascript-eval") then
            defines { "JAVASCRIPT_EVAL_ENABLED" }
        end
        if use_threads then
            defines { "PTHREAD_NO_RENAME" }
            buildoptions { "-sUSE_PTHREADS=1" }
        end
        if opt_bool("proxy-to-pthread") and use_threads and not opt_bool("dlink") then
            defines { "PROXY_TO_PTHREAD_ENABLED" }
        end
        if opt_bool("dlink") then
            defines { "WEB_DLINK_ENABLED" }
            buildoptions { "-sSIDE_MODULE=2", "-fvisibility=hidden" }
        end
        if opt_bool("wasm-simd") then
            buildoptions { "-msimd128" }
        end
        buildoptions { "-sSUPPORT_LONGJMP=wasm" }
        filter "platforms:wasm32"
            buildoptions { "-sMEMORY64=0" }
        filter "platforms:wasm64"
            buildoptions { "-sMEMORY64=1" }
        filter {}
    end

    -- -------------------------------------------------------------------------
    -- Architecture & GCC/Clang/MSVC common flags
    -- -------------------------------------------------------------------------
    if not is_msvc then
        buildoptions { "-ffp-contract=off" }
        if target_platform == "linuxbsd" then
            buildoptions { "-pipe" }
        end
        if not opts.no_sse42 then
            filter "platforms:x86_64"
                buildoptions { "-msse4.2", "-mpopcnt" }
            filter {}
        end
        filter "platforms:x86_32"
            buildoptions { "-msse2", "-mfpmath=sse", "-mstackrealign" }
        filter "platforms:rv64"
            buildoptions { "-march=rv64gc" }
        filter {}
    else
        if not opts.no_sse42 then
            filter "platforms:x86_64"
                buildoptions { "/d2archSSE42" }
            filter {}
        end
        filter "platforms:x86_32"
            buildoptions { "/arch:SSE2" }
        filter {}
    end

    -- LTO
    if lto_mode == "thin" and not is_msvc then
        buildoptions { "-flto=thin" }
    elseif lto_mode == "full" then
        if is_msvc then
            buildoptions { "/GL" }
        else
            buildoptions { "-flto" }
            if target_platform == "windows" and not use_llvm then
                buildoptions { "-fno-use-linker-plugin" }
            end
        end
    end

    -- Sanitizers
    if opt_bool("asan") then
        defines { "ASAN_ENABLED" }
        buildoptions { is_msvc and "/fsanitize=address" or "-fsanitize=address" }
    end
    if opt_bool("ubsan") and not is_msvc then
        defines { "UBSAN_ENABLED" }
        buildoptions { "-fsanitize=undefined" }
        if target_platform == "windows" then buildoptions { "-fno-sanitize=vptr" } end
    end
    if opt_bool("tsan") and not is_msvc then
        defines { "TSAN_ENABLED" }
        buildoptions { "-fsanitize=thread" }
    end
    if opt_bool("lsan") and not is_msvc then
        defines { "LSAN_ENABLED" }
        buildoptions { "-fsanitize=leak" }
    end

    -- Optimization & debug symbols per target configuration
    local opt_override = _OPTIONS["optimize"] or "auto"
    local sym_editor = opt_tri("debug-symbols", not is_production)
    local sym_tmpl_dbg = opt_tri("debug-symbols", is_dev_build)
    local sym_tmpl_rel = opt_tri("debug-symbols", false)

    local function apply_opt_and_syms(default_opt, want_syms)
        local chosen = (opt_override ~= "auto") and opt_override or (is_dev_build and "none" or default_opt)
        if target_platform == "web" and opt_override == "auto" and not is_dev_build then
            chosen = "size"
        end
        if chosen == "none" then
            optimize "Off"
        elseif chosen == "debug" then
            optimize "Debug"
        elseif chosen == "speed_trace" then
            optimize "On" -- -O2 on GCC/Clang, /O2 on MSVC
        elseif chosen == "speed" then
            optimize "Speed" -- -O3 on GCC/Clang, /O2 on MSVC
        elseif chosen == "size" or chosen == "size_extra" then
            optimize "Size" -- -Os on GCC/Clang, /O1 on MSVC
            if chosen == "size_extra" then
                defines { "SIZE_EXTRA" }
            end
        end

        if want_syms then
            symbols "On"
            if not is_msvc then
                if target_platform == "web" then
                    buildoptions { "-gdwarf-4", "-g3" }
                elseif target_platform == "windows" and not use_llvm then
                    buildoptions { "-gdwarf-5", is_dev_build and "-g3" or "-g2" }
                else
                    buildoptions { "-gdwarf-4", is_dev_build and "-g3" or "-g2" }
                end
            else
                buildoptions { "/FS" }
            end
        else
            if target_platform == "web" or target_platform == "ios" or target_platform == "visionos" then
                symbols "Default" -- Avoid passing bare '-s' to em++ or Apple static archives
            else
                symbols "Off"
            end
        end
    end

    filter "configurations:editor"
        apply_opt_and_syms("speed_trace", sym_editor)
    filter "configurations:template_debug"
        apply_opt_and_syms("speed_trace", sym_tmpl_dbg)
    filter "configurations:template_release"
        apply_opt_and_syms("speed", sym_tmpl_rel)
    filter {}
end

local function apply_engine_warnings()
    local w = _OPTIONS["warnings"] or "no"
    if w == "no" then
        warnings "Off"
    elseif w == "moderate" then
        warnings "Default"
        if not is_msvc then buildoptions { "-Wno-unused" } end
    elseif w == "all" then
        warnings "High"
    elseif w == "extra" then
        warnings "Extra"
        if not is_msvc then buildoptions { "-Wno-unused-parameter" } end
    end
    if opt_bool("werror") then
        flags { "FatalWarnings" }
    end
end

-- Helper for Mesa D3D12 include/define setup on Windows
local function apply_d3d12_mesa_settings()
    local suf = is_msvc and "-msvc" or (use_llvm and "-llvm" or "-gcc")
    local mesa_root = mesa_libs_base
    if os.isdir(mesa_libs_base .. "-" .. primary_arch .. suf) then
        mesa_root = mesa_libs_base .. "-" .. primary_arch .. suf
    end
    local mesa_dir = mesa_root .. "/godot-mesa"
    if os.isdir(mesa_dir) then
        local dirs = os.matchdirs(mesa_dir .. "/**")
        table.insert(dirs, 1, mesa_dir)
        for _, d in ipairs(dirs) do
            local norm = d:gsub("\\", "/")
            if not norm:find("/src/c11") then
                includedirs { norm }
            end
        end
    end
    includedirs {
        "thirdparty/directx_headers/include/directx",
        "thirdparty/directx_headers/include/dxguids",
        "thirdparty/d3d12ma",
    }
    defines {
        "WINDOWS_NO_FUTEX",
        "__STDC_CONSTANT_MACROS",
        "__STDC_FORMAT_MACROS",
        "__STDC_LIMIT_MACROS",
        'PACKAGE_VERSION="25.3.0"',
        'PACKAGE_BUGREPORT="https://gitlab.freedesktop.org/mesa/mesa/-/issues"',
        "PIPE_SUBSYSTEM_WINDOWS_USER",
        "_Static_assert=static_assert",
        "HAVE_STRUCT_TIMESPEC",
    }
    if is_msvc then
        defines {
            "_USE_MATH_DEFINES",
            "VC_EXTRALEAN",
            "_CRT_SECURE_NO_WARNINGS",
            "_CRT_SECURE_NO_DEPRECATE",
            "_SCL_SECURE_NO_WARNINGS",
            "_SCL_SECURE_NO_DEPRECATE",
            "_ALLOW_KEYWORD_MACROS",
        }
    else
        defines { "__REQUIRED_RPCNDR_H_VERSION__=475" }
    end
    if os.isdir(agility_sdk_path) then
        defines { "AGILITY_SDK_ENABLED" }
    end
    if os.isdir(pix_path) and (primary_arch == "x86_64" or primary_arch == "arm64") then
        includedirs { pix_path .. "/Include" }
        defines { "PIX_ENABLED" }
    end
end

-- =============================================================================
-- 5. THIRDPARTY & ISOLATED STATIC LIBRARIES (zero per-file filters for Ninja!)
-- =============================================================================

local linked_tp_libs = {}
local editor_only_tp_libs = {}
local cvtt_tp_libs = {}

local function register_tp(name, mode)
    if mode == "editor" then
        table.insert(editor_only_tp_libs, name)
    elseif mode == "cvtt" then
        table.insert(cvtt_tp_libs, name)
    else
        table.insert(linked_tp_libs, name)
    end
end

-- 1. ThorVG
project "tp_thorvg"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs {
        "thirdparty/thorvg/inc",
        "thirdparty/thorvg/src/common",
        "thirdparty/thorvg/src/renderer",
        "thirdparty/thorvg/src/renderer/sw_engine",
        "thirdparty/thorvg/src/loaders/svg",
        "thirdparty/thorvg/src/loaders/raw",
        "thirdparty/thorvg/src/loaders/external_png",
        "thirdparty/thorvg/src/loaders/external_jpg",
        "thirdparty/thorvg/src/loaders/external_webp",
        "thirdparty/libpng",
        "thirdparty/libjpeg-turbo/src",
        "thirdparty/libwebp/src",
        "thirdparty/zlib",
    }
    defines {
        "GODOT_MODULE",
        "TVG_STATIC",
        "THORVG_FILE_IO_SUPPORT",
        "THORVG_WEBP_LOADER_SUPPORT",
        "THORVG_JPG_LOADER_SUPPORT",
    }
    files {
        "thirdparty/thorvg/src/common/tvgColor.cpp",
        "thirdparty/thorvg/src/common/tvgCompressor.cpp",
        "thirdparty/thorvg/src/common/tvgMath.cpp",
        "thirdparty/thorvg/src/common/tvgStr.cpp",
        "thirdparty/thorvg/src/loaders/external_jpg/tvgJpgLoader.cpp",
        "thirdparty/thorvg/src/loaders/external_png/tvgPngLoader.cpp",
        "thirdparty/thorvg/src/loaders/external_webp/tvgWebpLoader.cpp",
        "thirdparty/thorvg/src/loaders/raw/tvgRawLoader.cpp",
        "thirdparty/thorvg/src/loaders/svg/tvgSvgCssStyle.cpp",
        "thirdparty/thorvg/src/loaders/svg/tvgSvgLoader.cpp",
        "thirdparty/thorvg/src/loaders/svg/tvgSvgPath.cpp",
        "thirdparty/thorvg/src/loaders/svg/tvgSvgSceneBuilder.cpp",
        "thirdparty/thorvg/src/loaders/svg/tvgSvgUtil.cpp",
        "thirdparty/thorvg/src/loaders/svg/tvgXmlParser.cpp",
        "thirdparty/thorvg/src/renderer/sw_engine/tvgSwFill.cpp",
        "thirdparty/thorvg/src/renderer/sw_engine/tvgSwImage.cpp",
        "thirdparty/thorvg/src/renderer/sw_engine/tvgSwMath.cpp",
        "thirdparty/thorvg/src/renderer/sw_engine/tvgSwMemPool.cpp",
        "thirdparty/thorvg/src/renderer/sw_engine/tvgSwPostEffect.cpp",
        "thirdparty/thorvg/src/renderer/sw_engine/tvgSwRaster.cpp",
        "thirdparty/thorvg/src/renderer/sw_engine/tvgSwRenderer.cpp",
        "thirdparty/thorvg/src/renderer/sw_engine/tvgSwRle.cpp",
        "thirdparty/thorvg/src/renderer/sw_engine/tvgSwShape.cpp",
        "thirdparty/thorvg/src/renderer/sw_engine/tvgSwStroke.cpp",
        "thirdparty/thorvg/src/renderer/tvgAccessor.cpp",
        "thirdparty/thorvg/src/renderer/tvgCanvas.cpp",
        "thirdparty/thorvg/src/renderer/tvgFill.cpp",
        "thirdparty/thorvg/src/renderer/tvgInitializer.cpp",
        "thirdparty/thorvg/src/renderer/tvgLoader.cpp",
        "thirdparty/thorvg/src/renderer/tvgPaint.cpp",
        "thirdparty/thorvg/src/renderer/tvgPicture.cpp",
        "thirdparty/thorvg/src/renderer/tvgRender.cpp",
        "thirdparty/thorvg/src/renderer/tvgScene.cpp",
        "thirdparty/thorvg/src/renderer/tvgShape.cpp",
        "thirdparty/thorvg/src/renderer/tvgTaskScheduler.cpp",
        "thirdparty/thorvg/src/renderer/tvgText.cpp",
    }
register_tp("tp_thorvg")

-- 2. libktx
project "tp_libktx"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs {
        "thirdparty/libktx/include",
        "thirdparty/libktx/utils",
        "thirdparty/libktx/lib",
        "thirdparty/libktx/other_include",
        "thirdparty/libktx/external",
        "thirdparty/basis_universal",
        "thirdparty/zstd",
    }
    defines {
        "GODOT_MODULE",
        "KHRONOS_STATIC=1",
    }
    if use_vulkan then
        includedirs { "thirdparty/vulkan/include" }
    else
        defines { "LIBKTX" }
    end
    -- Basis Universal encoder already compiles miniz in editor builds; templates need libktx's miniz
    filter "configurations:editor"
        defines { "MINIZ_HEADER_FILE_ONLY" }
    filter {}
    files {
        "thirdparty/libktx/external/dfdutils/colourspaces.c",
        "thirdparty/libktx/external/dfdutils/createdfd.c",
        "thirdparty/libktx/external/dfdutils/interpretdfd.c",
        "thirdparty/libktx/external/dfdutils/printdfd.c",
        "thirdparty/libktx/external/dfdutils/queries.c",
        "thirdparty/libktx/external/dfdutils/vk2dfd.c",
        "thirdparty/libktx/lib/basis_transcode.cpp",
        "thirdparty/libktx/lib/checkheader.c",
        "thirdparty/libktx/lib/filestream.c",
        "thirdparty/libktx/lib/hashlist.c",
        "thirdparty/libktx/lib/memstream.c",
        "thirdparty/libktx/lib/miniz_wrapper.cpp",
        "thirdparty/libktx/lib/swap.c",
        "thirdparty/libktx/lib/texture.c",
        "thirdparty/libktx/lib/texture1.c",
        "thirdparty/libktx/lib/texture2.c",
        "thirdparty/libktx/lib/vkformat_check.c",
        "thirdparty/libktx/lib/vkformat_check_variant.c",
        "thirdparty/libktx/lib/vkformat_typesize.c",
    }
register_tp("tp_libktx")

-- 3. Basis Universal (transcoder always; encoder in editor only)
project "tp_basis_universal"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs {
        "thirdparty/basis_universal",
        "thirdparty/tinyexr",
        "thirdparty/zstd",
        "thirdparty/zlib",
    }
    defines {
        "GODOT_MODULE",
        "BASISD_SUPPORT_KTX2_ZSTD=1",
        "BASISD_SUPPORT_ATC=0",
        "BASISD_SUPPORT_FXT1=0",
        "BASISD_SUPPORT_PVRTC1=0",
        "BASISD_SUPPORT_PVRTC2=0",
    }
    if not is_msvc then
        buildoptions { "-fno-strict-aliasing" }
    end
    files {
        "thirdparty/basis_universal/transcoder/basisu_transcoder.cpp",
    }
    filter "configurations:editor"
        files {
            "thirdparty/basis_universal/encoder/3rdparty/android_astc_decomp.cpp",
            "thirdparty/basis_universal/encoder/basisu_astc_hdr_6x6_enc.cpp",
            "thirdparty/basis_universal/encoder/basisu_astc_hdr_common.cpp",
            "thirdparty/basis_universal/encoder/basisu_backend.cpp",
            "thirdparty/basis_universal/encoder/basisu_basis_file.cpp",
            "thirdparty/basis_universal/encoder/basisu_bc7enc.cpp",
            "thirdparty/basis_universal/encoder/basisu_comp.cpp",
            "thirdparty/basis_universal/encoder/basisu_enc.cpp",
            "thirdparty/basis_universal/encoder/basisu_etc.cpp",
            "thirdparty/basis_universal/encoder/basisu_frontend.cpp",
            "thirdparty/basis_universal/encoder/basisu_gpu_texture.cpp",
            "thirdparty/basis_universal/encoder/basisu_kernels_sse.cpp",
            "thirdparty/basis_universal/encoder/basisu_opencl.cpp",
            "thirdparty/basis_universal/encoder/basisu_pvrtc1_4.cpp",
            "thirdparty/basis_universal/encoder/basisu_resample_filters.cpp",
            "thirdparty/basis_universal/encoder/basisu_resampler.cpp",
            "thirdparty/basis_universal/encoder/basisu_ssim.cpp",
            "thirdparty/basis_universal/encoder/basisu_uastc_enc.cpp",
            "thirdparty/basis_universal/encoder/basisu_uastc_hdr_4x4_enc.cpp",
            "thirdparty/basis_universal/encoder/jpgd.cpp",
            "thirdparty/basis_universal/encoder/pvpngreader.cpp",
        }
    filter {}
register_tp("tp_basis_universal")

-- 4. TinyEXR
project "tp_tinyexr"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs { "thirdparty/tinyexr", "thirdparty/zlib" }
    defines { "GODOT_MODULE", "TINYEXR_USE_THREAD", "TINYEXR_USE_MINIZ=0" }
    files {
        "thirdparty/tinyexr/tinyexr.cc",
    }
register_tp("tp_tinyexr")

-- 5. FreeType + HarfBuzz (combined archive resolves mutual FT <-> HB cycle & dead-strips unused HB subset TUs)
local ft_hb_defines = {
    "GODOT_MODULE",
    "FT2_BUILD_LIBRARY",
    "FT_CONFIG_OPTION_SYSTEM_ZLIB",
    "FT_CONFIG_OPTION_USE_HARFBUZZ",
    "FT_CONFIG_OPTION_USE_PNG",
    "HAVE_FREETYPE",
    "HAVE_ICU",
    "HAVE_ICU_BUILTIN",
    "HAVE_PNG",
    "HAVE_ZLIB",
    "U_STATIC_IMPLEMENTATION",
    "U_HAVE_LIB_SUFFIX=1",
    "U_LIB_SUFFIX_C_NAME=_godot",
}

project "tp_freetype_harfbuzz"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs {
        "thirdparty/freetype/include",
        "thirdparty/harfbuzz/src",
        "thirdparty/icu4c/common",
        "thirdparty/icu4c/i18n",
        "thirdparty/libpng",
        "thirdparty/zlib",
    }
    defines(ft_hb_defines)
    if use_brotli then
        includedirs { "thirdparty/brotli/include" }
        defines { "FT_CONFIG_OPTION_USE_BROTLI" }
    end
    if use_graphite then
        includedirs { "thirdparty/graphite/include" }
        defines { "HAVE_GRAPHITE2", "GRAPHITE2_STATIC" }
    end
    if target_platform == "linuxbsd" or target_platform == "android" or target_platform == "web" then
        defines { "HAVE_PTHREAD" }
    end
    files {
        "thirdparty/freetype/src/autofit/autofit.c",
        "thirdparty/freetype/src/base/ftbase.c",
        "thirdparty/freetype/src/base/ftbbox.c",
        "thirdparty/freetype/src/base/ftbdf.c",
        "thirdparty/freetype/src/base/ftbitmap.c",
        "thirdparty/freetype/src/base/ftcid.c",
        "thirdparty/freetype/src/base/ftdebug.c",
        "thirdparty/freetype/src/base/ftfstype.c",
        "thirdparty/freetype/src/base/ftgasp.c",
        "thirdparty/freetype/src/base/ftglyph.c",
        "thirdparty/freetype/src/base/ftgxval.c",
        "thirdparty/freetype/src/base/ftinit.c",
        "thirdparty/freetype/src/base/ftmm.c",
        "thirdparty/freetype/src/base/ftotval.c",
        "thirdparty/freetype/src/base/ftpatent.c",
        "thirdparty/freetype/src/base/ftpfr.c",
        "thirdparty/freetype/src/base/ftstroke.c",
        "thirdparty/freetype/src/base/ftsynth.c",
        "thirdparty/freetype/src/base/ftsystem.c",
        "thirdparty/freetype/src/base/fttype1.c",
        "thirdparty/freetype/src/base/ftwinfnt.c",
        "thirdparty/freetype/src/bdf/bdf.c",
        "thirdparty/freetype/src/bzip2/ftbzip2.c",
        "thirdparty/freetype/src/cache/ftcache.c",
        "thirdparty/freetype/src/cff/cff.c",
        "thirdparty/freetype/src/cid/type1cid.c",
        "thirdparty/freetype/src/gxvalid/gxvalid.c",
        "thirdparty/freetype/src/gzip/ftgzip.c",
        "thirdparty/freetype/src/lzw/ftlzw.c",
        "thirdparty/freetype/src/otvalid/otvalid.c",
        "thirdparty/freetype/src/pcf/pcf.c",
        "thirdparty/freetype/src/pfr/pfr.c",
        "thirdparty/freetype/src/psaux/psaux.c",
        "thirdparty/freetype/src/pshinter/pshinter.c",
        "thirdparty/freetype/src/psnames/psnames.c",
        "thirdparty/freetype/src/raster/raster.c",
        "thirdparty/freetype/src/sdf/sdf.c",
        "thirdparty/freetype/src/smooth/smooth.c",
        "thirdparty/freetype/src/svg/svg.c",
        "thirdparty/freetype/src/truetype/truetype.c",
        "thirdparty/freetype/src/type1/type1.c",
        "thirdparty/freetype/src/type42/type42.c",
        "thirdparty/freetype/src/winfonts/winfnt.c",
        "thirdparty/harfbuzz/src/OT/Var/VARC/VARC.cc",
        "thirdparty/harfbuzz/src/hb-aat-layout.cc",
        "thirdparty/harfbuzz/src/hb-aat-map.cc",
        "thirdparty/harfbuzz/src/hb-blob.cc",
        "thirdparty/harfbuzz/src/hb-buffer-serialize.cc",
        "thirdparty/harfbuzz/src/hb-buffer-verify.cc",
        "thirdparty/harfbuzz/src/hb-buffer.cc",
        "thirdparty/harfbuzz/src/hb-common.cc",
        "thirdparty/harfbuzz/src/hb-draw.cc",
        "thirdparty/harfbuzz/src/hb-face-builder.cc",
        "thirdparty/harfbuzz/src/hb-face.cc",
        "thirdparty/harfbuzz/src/hb-fallback-shape.cc",
        "thirdparty/harfbuzz/src/hb-font.cc",
        "thirdparty/harfbuzz/src/hb-ft.cc",
        "thirdparty/harfbuzz/src/hb-icu.cc",
        "thirdparty/harfbuzz/src/hb-map.cc",
        "thirdparty/harfbuzz/src/hb-number.cc",
        "thirdparty/harfbuzz/src/hb-ot-cff1-table.cc",
        "thirdparty/harfbuzz/src/hb-ot-cff2-table.cc",
        "thirdparty/harfbuzz/src/hb-ot-color.cc",
        "thirdparty/harfbuzz/src/hb-ot-face.cc",
        "thirdparty/harfbuzz/src/hb-ot-font.cc",
        "thirdparty/harfbuzz/src/hb-ot-layout.cc",
        "thirdparty/harfbuzz/src/hb-ot-map.cc",
        "thirdparty/harfbuzz/src/hb-ot-math.cc",
        "thirdparty/harfbuzz/src/hb-ot-meta.cc",
        "thirdparty/harfbuzz/src/hb-ot-metrics.cc",
        "thirdparty/harfbuzz/src/hb-ot-name.cc",
        "thirdparty/harfbuzz/src/hb-ot-shape-fallback.cc",
        "thirdparty/harfbuzz/src/hb-ot-shape-normalize.cc",
        "thirdparty/harfbuzz/src/hb-ot-shape.cc",
        "thirdparty/harfbuzz/src/hb-ot-shaper-arabic.cc",
        "thirdparty/harfbuzz/src/hb-ot-shaper-default.cc",
        "thirdparty/harfbuzz/src/hb-ot-shaper-hangul.cc",
        "thirdparty/harfbuzz/src/hb-ot-shaper-hebrew.cc",
        "thirdparty/harfbuzz/src/hb-ot-shaper-indic-table.cc",
        "thirdparty/harfbuzz/src/hb-ot-shaper-indic.cc",
        "thirdparty/harfbuzz/src/hb-ot-shaper-khmer.cc",
        "thirdparty/harfbuzz/src/hb-ot-shaper-myanmar.cc",
        "thirdparty/harfbuzz/src/hb-ot-shaper-syllabic.cc",
        "thirdparty/harfbuzz/src/hb-ot-shaper-thai.cc",
        "thirdparty/harfbuzz/src/hb-ot-shaper-use.cc",
        "thirdparty/harfbuzz/src/hb-ot-shaper-vowel-constraints.cc",
        "thirdparty/harfbuzz/src/hb-ot-tag.cc",
        "thirdparty/harfbuzz/src/hb-ot-var.cc",
        "thirdparty/harfbuzz/src/hb-outline.cc",
        "thirdparty/harfbuzz/src/hb-paint-bounded.cc",
        "thirdparty/harfbuzz/src/hb-paint-extents.cc",
        "thirdparty/harfbuzz/src/hb-paint.cc",
        "thirdparty/harfbuzz/src/hb-raster-draw.cc",
        "thirdparty/harfbuzz/src/hb-raster-image.cc",
        "thirdparty/harfbuzz/src/hb-raster-paint.cc",
        "thirdparty/harfbuzz/src/hb-raster.cc",
        "thirdparty/harfbuzz/src/hb-set.cc",
        "thirdparty/harfbuzz/src/hb-shape-plan.cc",
        "thirdparty/harfbuzz/src/hb-shape.cc",
        "thirdparty/harfbuzz/src/hb-shaper.cc",
        "thirdparty/harfbuzz/src/hb-static.cc",
        "thirdparty/harfbuzz/src/hb-style.cc",
        "thirdparty/harfbuzz/src/hb-subset.cc",
        "thirdparty/harfbuzz/src/hb-ucd.cc",
        "thirdparty/harfbuzz/src/hb-unicode.cc",
        "thirdparty/harfbuzz/src/hb-vector-draw.cc",
        "thirdparty/harfbuzz/src/hb-vector-paint-svg.cc",
        "thirdparty/harfbuzz/src/hb-vector-paint.cc",
        "thirdparty/harfbuzz/src/hb-vector-path.cc",
        "thirdparty/harfbuzz/src/hb-vector.cc",
        "thirdparty/harfbuzz/src/hb-zlib.cc",
    }
    if use_graphite then
        files { "thirdparty/harfbuzz/src/hb-graphite2.cc" }
    end
    if target_platform ~= "web" then
        files { "thirdparty/freetype/src/sfnt/sfnt.c" }
    end
register_tp("tp_freetype_harfbuzz")

if target_platform == "web" then
    -- On WebAssembly, sfnt.c must be compiled with -U__OPTIMIZE__ (modules/freetype/SCsub)
    project "tp_freetype_sfnt_wasm"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs {
            "thirdparty/freetype/include",
            "thirdparty/harfbuzz/src",
            "thirdparty/libpng",
            "thirdparty/zlib",
        }
        defines(ft_hb_defines)
        if use_brotli then
            includedirs { "thirdparty/brotli/include" }
            defines { "FT_CONFIG_OPTION_USE_BROTLI" }
        end
        buildoptions { "-U__OPTIMIZE__" }
        files { "thirdparty/freetype/src/sfnt/sfnt.c" }
    register_tp("tp_freetype_sfnt_wasm")
end

-- 6. Graphite2
if use_graphite then
    project "tp_graphite"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/graphite/src", "thirdparty/graphite/include" }
        defines { "GODOT_MODULE", "GRAPHITE2_STATIC", "GRAPHITE2_NTRACING", "GRAPHITE2_NFILEFACE" }
        files {
            "thirdparty/graphite/src/CmapCache.cpp",
            "thirdparty/graphite/src/Code.cpp",
            "thirdparty/graphite/src/Collider.cpp",
            "thirdparty/graphite/src/Decompressor.cpp",
            "thirdparty/graphite/src/Face.cpp",
            "thirdparty/graphite/src/FeatureMap.cpp",
            "thirdparty/graphite/src/FileFace.cpp",
            "thirdparty/graphite/src/Font.cpp",
            "thirdparty/graphite/src/GlyphCache.cpp",
            "thirdparty/graphite/src/GlyphFace.cpp",
            "thirdparty/graphite/src/Intervals.cpp",
            "thirdparty/graphite/src/Justifier.cpp",
            "thirdparty/graphite/src/NameTable.cpp",
            "thirdparty/graphite/src/Pass.cpp",
            "thirdparty/graphite/src/Position.cpp",
            "thirdparty/graphite/src/Segment.cpp",
            "thirdparty/graphite/src/Silf.cpp",
            "thirdparty/graphite/src/Slot.cpp",
            "thirdparty/graphite/src/Sparse.cpp",
            "thirdparty/graphite/src/TtfUtil.cpp",
            "thirdparty/graphite/src/UtfCodec.cpp",
            "thirdparty/graphite/src/gr_char_info.cpp",
            "thirdparty/graphite/src/gr_face.cpp",
            "thirdparty/graphite/src/gr_features.cpp",
            "thirdparty/graphite/src/gr_font.cpp",
            "thirdparty/graphite/src/gr_logging.cpp",
            "thirdparty/graphite/src/gr_segment.cpp",
            "thirdparty/graphite/src/gr_slot.cpp",
            "thirdparty/graphite/src/json.cpp",
        }
        if is_msvc then
            files { "thirdparty/graphite/src/call_machine.cpp" }
        else
            files { "thirdparty/graphite/src/direct_machine.cpp" }
        end
    register_tp("tp_graphite")
end

-- 7. ICU4C (archive dead-strips unreferenced ubiditransform.o; templates add icudata_stub.cpp)
project "tp_icu4c"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs { "thirdparty/icu4c/common", "thirdparty/icu4c/i18n" }
    defines {
        "GODOT_MODULE",
        "U_STATIC_IMPLEMENTATION",
        "U_COMMON_IMPLEMENTATION",
        "UCONFIG_NO_COLLATION",
        "UCONFIG_NO_CONVERSION",
        "UCONFIG_NO_FORMATTING",
        "UCONFIG_NO_SERVICE",
        "UCONFIG_NO_IDNA",
        "UCONFIG_NO_FILE_IO",
        "UCONFIG_NO_TRANSLITERATION",
        "UCONFIG_NO_REGULAR_EXPRESSIONS",
        "PKGDATA_MODE=static",
        "U_ENABLE_DYLOAD=0",
        "U_HAVE_LIB_SUFFIX=1",
        "U_LIB_SUFFIX_C_NAME=_godot",
    }
    files {
        "thirdparty/icu4c/common/appendable.cpp",
        "thirdparty/icu4c/common/bmpset.cpp",
        "thirdparty/icu4c/common/brkeng.cpp",
        "thirdparty/icu4c/common/brkiter.cpp",
        "thirdparty/icu4c/common/bytesinkutil.cpp",
        "thirdparty/icu4c/common/bytestream.cpp",
        "thirdparty/icu4c/common/bytestrie.cpp",
        "thirdparty/icu4c/common/bytestriebuilder.cpp",
        "thirdparty/icu4c/common/bytestrieiterator.cpp",
        "thirdparty/icu4c/common/caniter.cpp",
        "thirdparty/icu4c/common/characterproperties.cpp",
        "thirdparty/icu4c/common/chariter.cpp",
        "thirdparty/icu4c/common/charstr.cpp",
        "thirdparty/icu4c/common/cmemory.cpp",
        "thirdparty/icu4c/common/cstr.cpp",
        "thirdparty/icu4c/common/cstring.cpp",
        "thirdparty/icu4c/common/cwchar.cpp",
        "thirdparty/icu4c/common/dictbe.cpp",
        "thirdparty/icu4c/common/dictionarydata.cpp",
        "thirdparty/icu4c/common/dtintrv.cpp",
        "thirdparty/icu4c/common/edits.cpp",
        "thirdparty/icu4c/common/emojiprops.cpp",
        "thirdparty/icu4c/common/errorcode.cpp",
        "thirdparty/icu4c/common/filteredbrk.cpp",
        "thirdparty/icu4c/common/filterednormalizer2.cpp",
        "thirdparty/icu4c/common/fixedstring.cpp",
        "thirdparty/icu4c/common/icudataver.cpp",
        "thirdparty/icu4c/common/icuplug.cpp",
        "thirdparty/icu4c/common/loadednormalizer2impl.cpp",
        "thirdparty/icu4c/common/localebuilder.cpp",
        "thirdparty/icu4c/common/localematcher.cpp",
        "thirdparty/icu4c/common/localeprioritylist.cpp",
        "thirdparty/icu4c/common/locavailable.cpp",
        "thirdparty/icu4c/common/locbased.cpp",
        "thirdparty/icu4c/common/locdispnames.cpp",
        "thirdparty/icu4c/common/locdistance.cpp",
        "thirdparty/icu4c/common/locdspnm.cpp",
        "thirdparty/icu4c/common/locid.cpp",
        "thirdparty/icu4c/common/loclikely.cpp",
        "thirdparty/icu4c/common/loclikelysubtags.cpp",
        "thirdparty/icu4c/common/locmap.cpp",
        "thirdparty/icu4c/common/locresdata.cpp",
        "thirdparty/icu4c/common/locutil.cpp",
        "thirdparty/icu4c/common/lsr.cpp",
        "thirdparty/icu4c/common/lstmbe.cpp",
        "thirdparty/icu4c/common/messagepattern.cpp",
        "thirdparty/icu4c/common/mlbe.cpp",
        "thirdparty/icu4c/common/normalizer2.cpp",
        "thirdparty/icu4c/common/normalizer2impl.cpp",
        "thirdparty/icu4c/common/normlzr.cpp",
        "thirdparty/icu4c/common/parsepos.cpp",
        "thirdparty/icu4c/common/patternprops.cpp",
        "thirdparty/icu4c/common/pluralmap.cpp",
        "thirdparty/icu4c/common/propname.cpp",
        "thirdparty/icu4c/common/propsvec.cpp",
        "thirdparty/icu4c/common/punycode.cpp",
        "thirdparty/icu4c/common/putil.cpp",
        "thirdparty/icu4c/common/rbbi.cpp",
        "thirdparty/icu4c/common/rbbi_cache.cpp",
        "thirdparty/icu4c/common/rbbidata.cpp",
        "thirdparty/icu4c/common/rbbinode.cpp",
        "thirdparty/icu4c/common/rbbirb.cpp",
        "thirdparty/icu4c/common/rbbiscan.cpp",
        "thirdparty/icu4c/common/rbbisetb.cpp",
        "thirdparty/icu4c/common/rbbistbl.cpp",
        "thirdparty/icu4c/common/rbbitblb.cpp",
        "thirdparty/icu4c/common/resbund.cpp",
        "thirdparty/icu4c/common/resbund_cnv.cpp",
        "thirdparty/icu4c/common/resource.cpp",
        "thirdparty/icu4c/common/restrace.cpp",
        "thirdparty/icu4c/common/ruleiter.cpp",
        "thirdparty/icu4c/common/schriter.cpp",
        "thirdparty/icu4c/common/serv.cpp",
        "thirdparty/icu4c/common/servlk.cpp",
        "thirdparty/icu4c/common/servlkf.cpp",
        "thirdparty/icu4c/common/servls.cpp",
        "thirdparty/icu4c/common/servnotf.cpp",
        "thirdparty/icu4c/common/servrbf.cpp",
        "thirdparty/icu4c/common/servslkf.cpp",
        "thirdparty/icu4c/common/sharedobject.cpp",
        "thirdparty/icu4c/common/simpleformatter.cpp",
        "thirdparty/icu4c/common/static_unicode_sets.cpp",
        "thirdparty/icu4c/common/stringpiece.cpp",
        "thirdparty/icu4c/common/stringtriebuilder.cpp",
        "thirdparty/icu4c/common/uarrsort.cpp",
        "thirdparty/icu4c/common/ubidi.cpp",
        "thirdparty/icu4c/common/ubidi_props.cpp",
        "thirdparty/icu4c/common/ubidiln.cpp",
        "thirdparty/icu4c/common/ubiditransform.cpp",
        "thirdparty/icu4c/common/ubidiwrt.cpp",
        "thirdparty/icu4c/common/ubrk.cpp",
        "thirdparty/icu4c/common/ucase.cpp",
        "thirdparty/icu4c/common/ucasemap.cpp",
        "thirdparty/icu4c/common/ucasemap_titlecase_brkiter.cpp",
        "thirdparty/icu4c/common/ucat.cpp",
        "thirdparty/icu4c/common/uchar.cpp",
        "thirdparty/icu4c/common/ucharstrie.cpp",
        "thirdparty/icu4c/common/ucharstriebuilder.cpp",
        "thirdparty/icu4c/common/ucharstrieiterator.cpp",
        "thirdparty/icu4c/common/uchriter.cpp",
        "thirdparty/icu4c/common/ucln_cmn.cpp",
        "thirdparty/icu4c/common/ucmndata.cpp",
        "thirdparty/icu4c/common/ucnv.cpp",
        "thirdparty/icu4c/common/ucnv2022.cpp",
        "thirdparty/icu4c/common/ucnv_bld.cpp",
        "thirdparty/icu4c/common/ucnv_cb.cpp",
        "thirdparty/icu4c/common/ucnv_cnv.cpp",
        "thirdparty/icu4c/common/ucnv_ct.cpp",
        "thirdparty/icu4c/common/ucnv_err.cpp",
        "thirdparty/icu4c/common/ucnv_ext.cpp",
        "thirdparty/icu4c/common/ucnv_io.cpp",
        "thirdparty/icu4c/common/ucnv_lmb.cpp",
        "thirdparty/icu4c/common/ucnv_set.cpp",
        "thirdparty/icu4c/common/ucnv_u16.cpp",
        "thirdparty/icu4c/common/ucnv_u32.cpp",
        "thirdparty/icu4c/common/ucnv_u7.cpp",
        "thirdparty/icu4c/common/ucnv_u8.cpp",
        "thirdparty/icu4c/common/ucnvbocu.cpp",
        "thirdparty/icu4c/common/ucnvdisp.cpp",
        "thirdparty/icu4c/common/ucnvhz.cpp",
        "thirdparty/icu4c/common/ucnvisci.cpp",
        "thirdparty/icu4c/common/ucnvlat1.cpp",
        "thirdparty/icu4c/common/ucnvmbcs.cpp",
        "thirdparty/icu4c/common/ucnvscsu.cpp",
        "thirdparty/icu4c/common/ucnvsel.cpp",
        "thirdparty/icu4c/common/ucol_swp.cpp",
        "thirdparty/icu4c/common/ucptrie.cpp",
        "thirdparty/icu4c/common/ucurr.cpp",
        "thirdparty/icu4c/common/udata.cpp",
        "thirdparty/icu4c/common/udatamem.cpp",
        "thirdparty/icu4c/common/udataswp.cpp",
        "thirdparty/icu4c/common/uenum.cpp",
        "thirdparty/icu4c/common/uhash.cpp",
        "thirdparty/icu4c/common/uhash_us.cpp",
        "thirdparty/icu4c/common/uidna.cpp",
        "thirdparty/icu4c/common/uinit.cpp",
        "thirdparty/icu4c/common/uinvchar.cpp",
        "thirdparty/icu4c/common/uiter.cpp",
        "thirdparty/icu4c/common/ulist.cpp",
        "thirdparty/icu4c/common/uloc.cpp",
        "thirdparty/icu4c/common/uloc_keytype.cpp",
        "thirdparty/icu4c/common/uloc_tag.cpp",
        "thirdparty/icu4c/common/ulocale.cpp",
        "thirdparty/icu4c/common/ulocbuilder.cpp",
        "thirdparty/icu4c/common/umapfile.cpp",
        "thirdparty/icu4c/common/umath.cpp",
        "thirdparty/icu4c/common/umutablecptrie.cpp",
        "thirdparty/icu4c/common/umutex.cpp",
        "thirdparty/icu4c/common/unames.cpp",
        "thirdparty/icu4c/common/unifiedcache.cpp",
        "thirdparty/icu4c/common/unifilt.cpp",
        "thirdparty/icu4c/common/unifunct.cpp",
        "thirdparty/icu4c/common/uniset.cpp",
        "thirdparty/icu4c/common/uniset_closure.cpp",
        "thirdparty/icu4c/common/uniset_props.cpp",
        "thirdparty/icu4c/common/unisetspan.cpp",
        "thirdparty/icu4c/common/unistr.cpp",
        "thirdparty/icu4c/common/unistr_case.cpp",
        "thirdparty/icu4c/common/unistr_case_locale.cpp",
        "thirdparty/icu4c/common/unistr_cnv.cpp",
        "thirdparty/icu4c/common/unistr_props.cpp",
        "thirdparty/icu4c/common/unistr_titlecase_brkiter.cpp",
        "thirdparty/icu4c/common/unorm.cpp",
        "thirdparty/icu4c/common/unormcmp.cpp",
        "thirdparty/icu4c/common/uobject.cpp",
        "thirdparty/icu4c/common/uprops.cpp",
        "thirdparty/icu4c/common/ures_cnv.cpp",
        "thirdparty/icu4c/common/uresbund.cpp",
        "thirdparty/icu4c/common/uresdata.cpp",
        "thirdparty/icu4c/common/usc_impl.cpp",
        "thirdparty/icu4c/common/uscript.cpp",
        "thirdparty/icu4c/common/uscript_props.cpp",
        "thirdparty/icu4c/common/uset.cpp",
        "thirdparty/icu4c/common/uset_props.cpp",
        "thirdparty/icu4c/common/usetiter.cpp",
        "thirdparty/icu4c/common/usprep.cpp",
        "thirdparty/icu4c/common/ustack.cpp",
        "thirdparty/icu4c/common/ustr_cnv.cpp",
        "thirdparty/icu4c/common/ustr_titlecase_brkiter.cpp",
        "thirdparty/icu4c/common/ustr_wcs.cpp",
        "thirdparty/icu4c/common/ustrcase.cpp",
        "thirdparty/icu4c/common/ustrcase_locale.cpp",
        "thirdparty/icu4c/common/ustrenum.cpp",
        "thirdparty/icu4c/common/ustrfmt.cpp",
        "thirdparty/icu4c/common/ustring.cpp",
        "thirdparty/icu4c/common/ustrtrns.cpp",
        "thirdparty/icu4c/common/utext.cpp",
        "thirdparty/icu4c/common/utf_impl.cpp",
        "thirdparty/icu4c/common/util.cpp",
        "thirdparty/icu4c/common/util_props.cpp",
        "thirdparty/icu4c/common/utrace.cpp",
        "thirdparty/icu4c/common/utrie.cpp",
        "thirdparty/icu4c/common/utrie2.cpp",
        "thirdparty/icu4c/common/utrie2_builder.cpp",
        "thirdparty/icu4c/common/utrie_swap.cpp",
        "thirdparty/icu4c/common/uts46.cpp",
        "thirdparty/icu4c/common/utypes.cpp",
        "thirdparty/icu4c/common/uvector.cpp",
        "thirdparty/icu4c/common/uvectr32.cpp",
        "thirdparty/icu4c/common/uvectr64.cpp",
        "thirdparty/icu4c/common/wintz.cpp",
        "thirdparty/icu4c/i18n/scriptset.cpp",
        "thirdparty/icu4c/i18n/ucln_in.cpp",
        "thirdparty/icu4c/i18n/uspoof.cpp",
        "thirdparty/icu4c/i18n/uspoof_impl.cpp",
    }
    filter "configurations:template_debug or template_release"
        files { "modules/text_server_adv/icu_data/icudata_stub.cpp" }
    filter {}
register_tp("tp_icu4c")

-- 8. MSDFgen
project "tp_msdfgen"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs { "thirdparty/msdfgen", "thirdparty/freetype/include" }
    defines { "GODOT_MODULE", "MSDFGEN_PUBLIC=" }
    files {
        "thirdparty/msdfgen/core/Contour.cpp",
        "thirdparty/msdfgen/core/DistanceMapping.cpp",
        "thirdparty/msdfgen/core/EdgeHolder.cpp",
        "thirdparty/msdfgen/core/MSDFErrorCorrection.cpp",
        "thirdparty/msdfgen/core/Projection.cpp",
        "thirdparty/msdfgen/core/Scanline.cpp",
        "thirdparty/msdfgen/core/Shape.cpp",
        "thirdparty/msdfgen/core/contour-combiners.cpp",
        "thirdparty/msdfgen/core/convergent-curve-ordering.cpp",
        "thirdparty/msdfgen/core/edge-coloring.cpp",
        "thirdparty/msdfgen/core/edge-segments.cpp",
        "thirdparty/msdfgen/core/edge-selectors.cpp",
        "thirdparty/msdfgen/core/equation-solver.cpp",
        "thirdparty/msdfgen/core/msdf-error-correction.cpp",
        "thirdparty/msdfgen/core/msdfgen.cpp",
        "thirdparty/msdfgen/core/rasterization.cpp",
        "thirdparty/msdfgen/core/render-sdf.cpp",
        "thirdparty/msdfgen/core/sdf-error-estimation.cpp",
        "thirdparty/msdfgen/core/shape-description.cpp",
    }
register_tp("tp_msdfgen")

-- 9. libtheora (omitted on RISC-V; encoder included in editor only)
if has_theora then
    project "tp_libtheora"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/libtheora", "thirdparty/libogg", "thirdparty/libvorbis" }
        defines { "GODOT_MODULE" }
        filter "platforms:not rv64"
            files {
                "thirdparty/libtheora/bitpack.c",
                "thirdparty/libtheora/decinfo.c",
                "thirdparty/libtheora/decode.c",
                "thirdparty/libtheora/dequant.c",
                "thirdparty/libtheora/fragment.c",
                "thirdparty/libtheora/huffdec.c",
                "thirdparty/libtheora/idct.c",
                "thirdparty/libtheora/info.c",
                "thirdparty/libtheora/internal.c",
                "thirdparty/libtheora/quant.c",
                "thirdparty/libtheora/state.c",
            }
        filter { "platforms:not rv64", "configurations:editor" }
            files {
                "thirdparty/libtheora/analyze.c",
                "thirdparty/libtheora/encfrag.c",
                "thirdparty/libtheora/encinfo.c",
                "thirdparty/libtheora/encode.c",
                "thirdparty/libtheora/enquant.c",
                "thirdparty/libtheora/fdct.c",
                "thirdparty/libtheora/huffenc.c",
                "thirdparty/libtheora/mathops.c",
                "thirdparty/libtheora/mcenc.c",
                "thirdparty/libtheora/rate.c",
                "thirdparty/libtheora/tokenize.c",
            }
        filter {}
        if target_platform == "linuxbsd" or target_platform == "macos" then
            filter "platforms:x86_64 or x86_32"
                defines { "OC_X86_ASM" }
                files {
                    "thirdparty/libtheora/x86/mmxfrag.c",
                    "thirdparty/libtheora/x86/mmxidct.c",
                    "thirdparty/libtheora/x86/mmxstate.c",
                    "thirdparty/libtheora/x86/sse2idct.c",
                    "thirdparty/libtheora/x86/x86cpu.c",
                    "thirdparty/libtheora/x86/x86state.c",
                }
            filter { "platforms:x86_64 or x86_32", "configurations:editor" }
                files {
                    "thirdparty/libtheora/x86/mmxencfrag.c",
                    "thirdparty/libtheora/x86/mmxfdct.c",
                    "thirdparty/libtheora/x86/sse2encfrag.c",
                    "thirdparty/libtheora/x86/sse2fdct.c",
                    "thirdparty/libtheora/x86/x86enc.c",
                    "thirdparty/libtheora/x86/x86enquant.c",
                }
            filter {}
        elseif target_platform == "windows" then
            if is_msvc then
                filter "platforms:x86_32"
                    defines { "OC_X86_ASM" }
                    files {
                        "thirdparty/libtheora/x86_vc/mmxfrag.c",
                        "thirdparty/libtheora/x86_vc/mmxidct.c",
                        "thirdparty/libtheora/x86_vc/mmxstate.c",
                        "thirdparty/libtheora/x86_vc/x86cpu.c",
                        "thirdparty/libtheora/x86_vc/x86state.c",
                    }
                filter { "platforms:x86_32", "configurations:editor" }
                    files {
                        "thirdparty/libtheora/x86_vc/mmxencfrag.c",
                        "thirdparty/libtheora/x86_vc/mmxfdct.c",
                        "thirdparty/libtheora/x86_vc/x86enc.c",
                    }
                filter {}
            else
                filter "platforms:x86_32"
                    defines { "OC_X86_ASM" }
                    files {
                        "thirdparty/libtheora/x86/mmxfrag.c",
                        "thirdparty/libtheora/x86/mmxidct.c",
                        "thirdparty/libtheora/x86/mmxstate.c",
                        "thirdparty/libtheora/x86/sse2idct.c",
                        "thirdparty/libtheora/x86/x86cpu.c",
                        "thirdparty/libtheora/x86/x86state.c",
                    }
                filter { "platforms:x86_32", "configurations:editor" }
                    files {
                        "thirdparty/libtheora/x86/mmxencfrag.c",
                        "thirdparty/libtheora/x86/mmxfdct.c",
                        "thirdparty/libtheora/x86/sse2encfrag.c",
                        "thirdparty/libtheora/x86/sse2fdct.c",
                        "thirdparty/libtheora/x86/x86enc.c",
                        "thirdparty/libtheora/x86/x86enquant.c",
                    }
                filter {}
            end
        end
    register_tp("tp_libtheora")
end

-- 10. libvorbis (encoder files in editor only)
project "tp_libvorbis"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs { "thirdparty/libvorbis", "thirdparty/libogg" }
    defines { "GODOT_MODULE" }
    files {
        "thirdparty/libvorbis/bitrate.c",
        "thirdparty/libvorbis/block.c",
        "thirdparty/libvorbis/codebook.c",
        "thirdparty/libvorbis/envelope.c",
        "thirdparty/libvorbis/floor0.c",
        "thirdparty/libvorbis/floor1.c",
        "thirdparty/libvorbis/info.c",
        "thirdparty/libvorbis/lookup.c",
        "thirdparty/libvorbis/lpc.c",
        "thirdparty/libvorbis/lsp.c",
        "thirdparty/libvorbis/mapping0.c",
        "thirdparty/libvorbis/mdct.c",
        "thirdparty/libvorbis/psy.c",
        "thirdparty/libvorbis/registry.c",
        "thirdparty/libvorbis/res0.c",
        "thirdparty/libvorbis/sharedbook.c",
        "thirdparty/libvorbis/smallft.c",
        "thirdparty/libvorbis/synthesis.c",
        "thirdparty/libvorbis/vorbisfile.c",
        "thirdparty/libvorbis/window.c",
    }
    filter "configurations:editor"
        files {
            "thirdparty/libvorbis/analysis.c",
            "thirdparty/libvorbis/vorbisenc.c",
        }
    filter {}
register_tp("tp_libvorbis")

-- 11. libogg
project "tp_libogg"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs { "thirdparty/libogg" }
    defines { "GODOT_MODULE" }
    files {
        "thirdparty/libogg/bitwise.c",
        "thirdparty/libogg/framing.c",
    }
register_tp("tp_libogg")

-- 12-14. libjpeg-turbo (common + 8-bit pass + 12-bit pass)
project "tp_libjpeg_common"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs { "thirdparty/libjpeg-turbo/src" }
    defines { "GODOT_MODULE" }
    files {
        "thirdparty/libjpeg-turbo/src/jaricom.c",
        "thirdparty/libjpeg-turbo/src/jcapimin.c",
        "thirdparty/libjpeg-turbo/src/jcarith.c",
        "thirdparty/libjpeg-turbo/src/jchuff.c",
        "thirdparty/libjpeg-turbo/src/jcicc.c",
        "thirdparty/libjpeg-turbo/src/jcinit.c",
        "thirdparty/libjpeg-turbo/src/jcmarker.c",
        "thirdparty/libjpeg-turbo/src/jcmaster.c",
        "thirdparty/libjpeg-turbo/src/jcomapi.c",
        "thirdparty/libjpeg-turbo/src/jcparam.c",
        "thirdparty/libjpeg-turbo/src/jcphuff.c",
        "thirdparty/libjpeg-turbo/src/jctrans.c",
        "thirdparty/libjpeg-turbo/src/jdapimin.c",
        "thirdparty/libjpeg-turbo/src/jdarith.c",
        "thirdparty/libjpeg-turbo/src/jdatadst-tj.c",
        "thirdparty/libjpeg-turbo/src/jdatadst.c",
        "thirdparty/libjpeg-turbo/src/jdatasrc-tj.c",
        "thirdparty/libjpeg-turbo/src/jdatasrc.c",
        "thirdparty/libjpeg-turbo/src/jdhuff.c",
        "thirdparty/libjpeg-turbo/src/jdicc.c",
        "thirdparty/libjpeg-turbo/src/jdinput.c",
        "thirdparty/libjpeg-turbo/src/jdmarker.c",
        "thirdparty/libjpeg-turbo/src/jdmaster.c",
        "thirdparty/libjpeg-turbo/src/jdphuff.c",
        "thirdparty/libjpeg-turbo/src/jdtrans.c",
        "thirdparty/libjpeg-turbo/src/jerror.c",
        "thirdparty/libjpeg-turbo/src/jfdctflt.c",
        "thirdparty/libjpeg-turbo/src/jmemmgr.c",
        "thirdparty/libjpeg-turbo/src/jmemnobs.c",
        "thirdparty/libjpeg-turbo/src/jpeg_nbits.c",
        "thirdparty/libjpeg-turbo/src/transupp.c",
        "thirdparty/libjpeg-turbo/src/turbojpeg.c",
    }
register_tp("tp_libjpeg_common")

project "tp_libjpeg_8"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs { "thirdparty/libjpeg-turbo/src" }
    defines { "GODOT_MODULE", "BITS_IN_JSAMPLE=8" }
    files {
        "thirdparty/libjpeg-turbo/src/jcapistd.c",
        "thirdparty/libjpeg-turbo/src/jccoefct.c",
        "thirdparty/libjpeg-turbo/src/jccolor.c",
        "thirdparty/libjpeg-turbo/src/jcdctmgr.c",
        "thirdparty/libjpeg-turbo/src/jcmainct.c",
        "thirdparty/libjpeg-turbo/src/jcprepct.c",
        "thirdparty/libjpeg-turbo/src/jcsample.c",
        "thirdparty/libjpeg-turbo/src/jdapistd.c",
        "thirdparty/libjpeg-turbo/src/jdcoefct.c",
        "thirdparty/libjpeg-turbo/src/jdcolor.c",
        "thirdparty/libjpeg-turbo/src/jddctmgr.c",
        "thirdparty/libjpeg-turbo/src/jdmainct.c",
        "thirdparty/libjpeg-turbo/src/jdmerge.c",
        "thirdparty/libjpeg-turbo/src/jdpostct.c",
        "thirdparty/libjpeg-turbo/src/jdsample.c",
        "thirdparty/libjpeg-turbo/src/jfdctfst.c",
        "thirdparty/libjpeg-turbo/src/jfdctint.c",
        "thirdparty/libjpeg-turbo/src/jidctflt.c",
        "thirdparty/libjpeg-turbo/src/jidctfst.c",
        "thirdparty/libjpeg-turbo/src/jidctint.c",
        "thirdparty/libjpeg-turbo/src/jidctred.c",
        "thirdparty/libjpeg-turbo/src/jquant1.c",
        "thirdparty/libjpeg-turbo/src/jquant2.c",
        "thirdparty/libjpeg-turbo/src/jutils.c",
    }
register_tp("tp_libjpeg_8")

project "tp_libjpeg_12"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs { "thirdparty/libjpeg-turbo/src" }
    defines { "GODOT_MODULE", "BITS_IN_JSAMPLE=12" }
    files {
        "thirdparty/libjpeg-turbo/src/jcapistd.c",
        "thirdparty/libjpeg-turbo/src/jccoefct.c",
        "thirdparty/libjpeg-turbo/src/jccolor.c",
        "thirdparty/libjpeg-turbo/src/jcdctmgr.c",
        "thirdparty/libjpeg-turbo/src/jcmainct.c",
        "thirdparty/libjpeg-turbo/src/jcprepct.c",
        "thirdparty/libjpeg-turbo/src/jcsample.c",
        "thirdparty/libjpeg-turbo/src/jdapistd.c",
        "thirdparty/libjpeg-turbo/src/jdcoefct.c",
        "thirdparty/libjpeg-turbo/src/jdcolor.c",
        "thirdparty/libjpeg-turbo/src/jddctmgr.c",
        "thirdparty/libjpeg-turbo/src/jdmainct.c",
        "thirdparty/libjpeg-turbo/src/jdmerge.c",
        "thirdparty/libjpeg-turbo/src/jdpostct.c",
        "thirdparty/libjpeg-turbo/src/jdsample.c",
        "thirdparty/libjpeg-turbo/src/jfdctfst.c",
        "thirdparty/libjpeg-turbo/src/jfdctint.c",
        "thirdparty/libjpeg-turbo/src/jidctflt.c",
        "thirdparty/libjpeg-turbo/src/jidctfst.c",
        "thirdparty/libjpeg-turbo/src/jidctint.c",
        "thirdparty/libjpeg-turbo/src/jidctred.c",
        "thirdparty/libjpeg-turbo/src/jquant1.c",
        "thirdparty/libjpeg-turbo/src/jquant2.c",
        "thirdparty/libjpeg-turbo/src/jutils.c",
    }
register_tp("tp_libjpeg_12")

-- 15. libwebp
project "tp_libwebp"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs { "thirdparty/libwebp", "thirdparty/libwebp/src" }
    defines { "GODOT_MODULE" }
    files {
        "thirdparty/libwebp/sharpyuv/sharpyuv.c",
        "thirdparty/libwebp/sharpyuv/sharpyuv_cpu.c",
        "thirdparty/libwebp/sharpyuv/sharpyuv_csp.c",
        "thirdparty/libwebp/sharpyuv/sharpyuv_dsp.c",
        "thirdparty/libwebp/sharpyuv/sharpyuv_gamma.c",
        "thirdparty/libwebp/sharpyuv/sharpyuv_neon.c",
        "thirdparty/libwebp/sharpyuv/sharpyuv_sse2.c",
        "thirdparty/libwebp/src/dec/alpha_dec.c",
        "thirdparty/libwebp/src/dec/buffer_dec.c",
        "thirdparty/libwebp/src/dec/frame_dec.c",
        "thirdparty/libwebp/src/dec/idec_dec.c",
        "thirdparty/libwebp/src/dec/io_dec.c",
        "thirdparty/libwebp/src/dec/quant_dec.c",
        "thirdparty/libwebp/src/dec/tree_dec.c",
        "thirdparty/libwebp/src/dec/vp8_dec.c",
        "thirdparty/libwebp/src/dec/vp8l_dec.c",
        "thirdparty/libwebp/src/dec/webp_dec.c",
        "thirdparty/libwebp/src/demux/anim_decode.c",
        "thirdparty/libwebp/src/demux/demux.c",
        "thirdparty/libwebp/src/dsp/alpha_processing.c",
        "thirdparty/libwebp/src/dsp/alpha_processing_mips_dsp_r2.c",
        "thirdparty/libwebp/src/dsp/alpha_processing_neon.c",
        "thirdparty/libwebp/src/dsp/alpha_processing_sse2.c",
        "thirdparty/libwebp/src/dsp/alpha_processing_sse41.c",
        "thirdparty/libwebp/src/dsp/cost.c",
        "thirdparty/libwebp/src/dsp/cost_mips32.c",
        "thirdparty/libwebp/src/dsp/cost_mips_dsp_r2.c",
        "thirdparty/libwebp/src/dsp/cost_neon.c",
        "thirdparty/libwebp/src/dsp/cost_sse2.c",
        "thirdparty/libwebp/src/dsp/cpu.c",
        "thirdparty/libwebp/src/dsp/dec.c",
        "thirdparty/libwebp/src/dsp/dec_clip_tables.c",
        "thirdparty/libwebp/src/dsp/dec_mips32.c",
        "thirdparty/libwebp/src/dsp/dec_mips_dsp_r2.c",
        "thirdparty/libwebp/src/dsp/dec_msa.c",
        "thirdparty/libwebp/src/dsp/dec_neon.c",
        "thirdparty/libwebp/src/dsp/dec_sse2.c",
        "thirdparty/libwebp/src/dsp/dec_sse41.c",
        "thirdparty/libwebp/src/dsp/enc.c",
        "thirdparty/libwebp/src/dsp/enc_mips32.c",
        "thirdparty/libwebp/src/dsp/enc_mips_dsp_r2.c",
        "thirdparty/libwebp/src/dsp/enc_msa.c",
        "thirdparty/libwebp/src/dsp/enc_neon.c",
        "thirdparty/libwebp/src/dsp/enc_sse2.c",
        "thirdparty/libwebp/src/dsp/enc_sse41.c",
        "thirdparty/libwebp/src/dsp/filters.c",
        "thirdparty/libwebp/src/dsp/filters_mips_dsp_r2.c",
        "thirdparty/libwebp/src/dsp/filters_msa.c",
        "thirdparty/libwebp/src/dsp/filters_neon.c",
        "thirdparty/libwebp/src/dsp/filters_sse2.c",
        "thirdparty/libwebp/src/dsp/lossless.c",
        "thirdparty/libwebp/src/dsp/lossless_avx2.c",
        "thirdparty/libwebp/src/dsp/lossless_enc.c",
        "thirdparty/libwebp/src/dsp/lossless_enc_avx2.c",
        "thirdparty/libwebp/src/dsp/lossless_enc_mips32.c",
        "thirdparty/libwebp/src/dsp/lossless_enc_mips_dsp_r2.c",
        "thirdparty/libwebp/src/dsp/lossless_enc_msa.c",
        "thirdparty/libwebp/src/dsp/lossless_enc_neon.c",
        "thirdparty/libwebp/src/dsp/lossless_enc_sse2.c",
        "thirdparty/libwebp/src/dsp/lossless_enc_sse41.c",
        "thirdparty/libwebp/src/dsp/lossless_mips_dsp_r2.c",
        "thirdparty/libwebp/src/dsp/lossless_msa.c",
        "thirdparty/libwebp/src/dsp/lossless_neon.c",
        "thirdparty/libwebp/src/dsp/lossless_sse2.c",
        "thirdparty/libwebp/src/dsp/lossless_sse41.c",
        "thirdparty/libwebp/src/dsp/rescaler.c",
        "thirdparty/libwebp/src/dsp/rescaler_mips32.c",
        "thirdparty/libwebp/src/dsp/rescaler_mips_dsp_r2.c",
        "thirdparty/libwebp/src/dsp/rescaler_msa.c",
        "thirdparty/libwebp/src/dsp/rescaler_neon.c",
        "thirdparty/libwebp/src/dsp/rescaler_sse2.c",
        "thirdparty/libwebp/src/dsp/ssim.c",
        "thirdparty/libwebp/src/dsp/ssim_sse2.c",
        "thirdparty/libwebp/src/dsp/upsampling.c",
        "thirdparty/libwebp/src/dsp/upsampling_mips_dsp_r2.c",
        "thirdparty/libwebp/src/dsp/upsampling_msa.c",
        "thirdparty/libwebp/src/dsp/upsampling_neon.c",
        "thirdparty/libwebp/src/dsp/upsampling_sse2.c",
        "thirdparty/libwebp/src/dsp/upsampling_sse41.c",
        "thirdparty/libwebp/src/dsp/yuv.c",
        "thirdparty/libwebp/src/dsp/yuv_mips32.c",
        "thirdparty/libwebp/src/dsp/yuv_mips_dsp_r2.c",
        "thirdparty/libwebp/src/dsp/yuv_neon.c",
        "thirdparty/libwebp/src/dsp/yuv_sse2.c",
        "thirdparty/libwebp/src/dsp/yuv_sse41.c",
        "thirdparty/libwebp/src/enc/alpha_enc.c",
        "thirdparty/libwebp/src/enc/analysis_enc.c",
        "thirdparty/libwebp/src/enc/backward_references_cost_enc.c",
        "thirdparty/libwebp/src/enc/backward_references_enc.c",
        "thirdparty/libwebp/src/enc/config_enc.c",
        "thirdparty/libwebp/src/enc/cost_enc.c",
        "thirdparty/libwebp/src/enc/filter_enc.c",
        "thirdparty/libwebp/src/enc/frame_enc.c",
        "thirdparty/libwebp/src/enc/histogram_enc.c",
        "thirdparty/libwebp/src/enc/iterator_enc.c",
        "thirdparty/libwebp/src/enc/near_lossless_enc.c",
        "thirdparty/libwebp/src/enc/picture_csp_enc.c",
        "thirdparty/libwebp/src/enc/picture_enc.c",
        "thirdparty/libwebp/src/enc/picture_psnr_enc.c",
        "thirdparty/libwebp/src/enc/picture_rescale_enc.c",
        "thirdparty/libwebp/src/enc/picture_tools_enc.c",
        "thirdparty/libwebp/src/enc/predictor_enc.c",
        "thirdparty/libwebp/src/enc/quant_enc.c",
        "thirdparty/libwebp/src/enc/syntax_enc.c",
        "thirdparty/libwebp/src/enc/token_enc.c",
        "thirdparty/libwebp/src/enc/tree_enc.c",
        "thirdparty/libwebp/src/enc/vp8l_enc.c",
        "thirdparty/libwebp/src/enc/webp_enc.c",
        "thirdparty/libwebp/src/mux/anim_encode.c",
        "thirdparty/libwebp/src/mux/muxedit.c",
        "thirdparty/libwebp/src/mux/muxinternal.c",
        "thirdparty/libwebp/src/mux/muxread.c",
        "thirdparty/libwebp/src/utils/bit_reader_utils.c",
        "thirdparty/libwebp/src/utils/bit_writer_utils.c",
        "thirdparty/libwebp/src/utils/color_cache_utils.c",
        "thirdparty/libwebp/src/utils/filters_utils.c",
        "thirdparty/libwebp/src/utils/huffman_encode_utils.c",
        "thirdparty/libwebp/src/utils/huffman_utils.c",
        "thirdparty/libwebp/src/utils/palette.c",
        "thirdparty/libwebp/src/utils/quant_levels_dec_utils.c",
        "thirdparty/libwebp/src/utils/quant_levels_utils.c",
        "thirdparty/libwebp/src/utils/random_utils.c",
        "thirdparty/libwebp/src/utils/rescaler_utils.c",
        "thirdparty/libwebp/src/utils/thread_utils.c",
        "thirdparty/libwebp/src/utils/utils.c",
    }
register_tp("tp_libwebp")

-- 16-17. PCRE2 (16-bit and 32-bit passes; JIT disabled on iOS/visionOS/Web/rv64)
for _, width in ipairs({ 16, 32 }) do
    local pname = "tp_pcre2_" .. tostring(width)
    project(pname)
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/pcre2/src" }
        defines {
            "GODOT_MODULE",
            "PCRE2_STATIC",
            "HAVE_CONFIG_H",
            "SUPPORT_UNICODE",
            "PCRE2_CODE_UNIT_WIDTH=" .. tostring(width),
        }
        if has_pcre2_jit then
            filter "platforms:not rv64"
                defines { "SUPPORT_JIT" }
            filter {}
        end
        files {
            "thirdparty/pcre2/src/pcre2_auto_possess.c",
            "thirdparty/pcre2/src/pcre2_chartables.c",
            "thirdparty/pcre2/src/pcre2_chkdint.c",
            "thirdparty/pcre2/src/pcre2_compile.c",
            "thirdparty/pcre2/src/pcre2_compile_cgroup.c",
            "thirdparty/pcre2/src/pcre2_compile_class.c",
            "thirdparty/pcre2/src/pcre2_config.c",
            "thirdparty/pcre2/src/pcre2_context.c",
            "thirdparty/pcre2/src/pcre2_convert.c",
            "thirdparty/pcre2/src/pcre2_dfa_match.c",
            "thirdparty/pcre2/src/pcre2_error.c",
            "thirdparty/pcre2/src/pcre2_extuni.c",
            "thirdparty/pcre2/src/pcre2_find_bracket.c",
            "thirdparty/pcre2/src/pcre2_jit_compile.c",
            "thirdparty/pcre2/src/pcre2_maketables.c",
            "thirdparty/pcre2/src/pcre2_match.c",
            "thirdparty/pcre2/src/pcre2_match_data.c",
            "thirdparty/pcre2/src/pcre2_match_next.c",
            "thirdparty/pcre2/src/pcre2_newline.c",
            "thirdparty/pcre2/src/pcre2_ord2utf.c",
            "thirdparty/pcre2/src/pcre2_pattern_info.c",
            "thirdparty/pcre2/src/pcre2_script_run.c",
            "thirdparty/pcre2/src/pcre2_serialize.c",
            "thirdparty/pcre2/src/pcre2_string_utils.c",
            "thirdparty/pcre2/src/pcre2_study.c",
            "thirdparty/pcre2/src/pcre2_substitute.c",
            "thirdparty/pcre2/src/pcre2_substring.c",
            "thirdparty/pcre2/src/pcre2_tables.c",
            "thirdparty/pcre2/src/pcre2_ucd.c",
            "thirdparty/pcre2/src/pcre2_valid_utf.c",
            "thirdparty/pcre2/src/pcre2_xclass.c",
        }
    register_tp(pname)
end

-- 18. MbedTLS + TF-PSA-Crypto
local mbedtls_defs = (is_msvc and action_name == "ninja")
    and { "GODOT_MODULE", "TF_PSA_CRYPTO_CONFIG_FILE=<godot_psa_config.h>", "MBEDTLS_CONFIG_FILE=<godot_mbedtls_config.h>" }
    or  { "GODOT_MODULE", 'TF_PSA_CRYPTO_CONFIG_FILE="godot_psa_config.h"', 'MBEDTLS_CONFIG_FILE="godot_mbedtls_config.h"' }

project "tp_mbedtls"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs {
        "thirdparty/mbedtls/godot",
        "thirdparty/mbedtls/include",
        "thirdparty/mbedtls/library",
        "thirdparty/mbedtls/tf-psa-crypto/include",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/include",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src",
        "thirdparty/mbedtls/tf-psa-crypto/core",
        "thirdparty/mbedtls/tf-psa-crypto/dispatch",
        "thirdparty/mbedtls/tf-psa-crypto/extras",
        "thirdparty/mbedtls/tf-psa-crypto/platform",
        "thirdparty/mbedtls/tf-psa-crypto/utilities",
        "thirdparty/zlib",
    }
    defines(mbedtls_defs)
    files {
        "thirdparty/mbedtls/library/debug.c",
        "thirdparty/mbedtls/library/error.c",
        "thirdparty/mbedtls/library/mbedtls_config.c",
        "thirdparty/mbedtls/library/mps_reader.c",
        "thirdparty/mbedtls/library/mps_trace.c",
        "thirdparty/mbedtls/library/net_sockets.c",
        "thirdparty/mbedtls/library/pkcs7.c",
        "thirdparty/mbedtls/library/ssl_cache.c",
        "thirdparty/mbedtls/library/ssl_ciphersuites.c",
        "thirdparty/mbedtls/library/ssl_client.c",
        "thirdparty/mbedtls/library/ssl_cookie.c",
        "thirdparty/mbedtls/library/ssl_debug_helpers_generated.c",
        "thirdparty/mbedtls/library/ssl_msg.c",
        "thirdparty/mbedtls/library/ssl_ticket.c",
        "thirdparty/mbedtls/library/ssl_tls.c",
        "thirdparty/mbedtls/library/ssl_tls12_client.c",
        "thirdparty/mbedtls/library/ssl_tls12_server.c",
        "thirdparty/mbedtls/library/ssl_tls13_client.c",
        "thirdparty/mbedtls/library/ssl_tls13_generic.c",
        "thirdparty/mbedtls/library/ssl_tls13_keys.c",
        "thirdparty/mbedtls/library/ssl_tls13_server.c",
        "thirdparty/mbedtls/library/timing.c",
        "thirdparty/mbedtls/library/version.c",
        "thirdparty/mbedtls/library/version_features.c",
        "thirdparty/mbedtls/library/x509.c",
        "thirdparty/mbedtls/library/x509_create.c",
        "thirdparty/mbedtls/library/x509_crl.c",
        "thirdparty/mbedtls/library/x509_crt.c",
        "thirdparty/mbedtls/library/x509_csr.c",
        "thirdparty/mbedtls/library/x509_oid.c",
        "thirdparty/mbedtls/library/x509write.c",
        "thirdparty/mbedtls/library/x509write_crt.c",
        "thirdparty/mbedtls/library/x509write_csr.c",
        "thirdparty/mbedtls/tf-psa-crypto/core/psa_crypto.c",
        "thirdparty/mbedtls/tf-psa-crypto/core/psa_crypto_client.c",
        "thirdparty/mbedtls/tf-psa-crypto/core/psa_crypto_driver_wrappers_no_static.c",
        "thirdparty/mbedtls/tf-psa-crypto/core/psa_crypto_random.c",
        "thirdparty/mbedtls/tf-psa-crypto/core/psa_crypto_slot_management.c",
        "thirdparty/mbedtls/tf-psa-crypto/core/psa_crypto_storage.c",
        "thirdparty/mbedtls/tf-psa-crypto/core/psa_its_file.c",
        "thirdparty/mbedtls/tf-psa-crypto/core/psa_util.c",
        "thirdparty/mbedtls/tf-psa-crypto/core/tf_psa_crypto_config.c",
        "thirdparty/mbedtls/tf-psa-crypto/core/tf_psa_crypto_version.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/aes.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/aesce.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/aesni.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/aria.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/bignum.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/bignum_core.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/bignum_mod.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/bignum_mod_raw.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/block_cipher.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/camellia.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/ccm.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/chacha20.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/chacha20_neon.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/chachapoly.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/cipher.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/cipher_wrap.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/cmac.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/ctr_drbg.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/ecdsa.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/ecjpake.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/ecp.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/ecp_curves.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/ecp_curves_new.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/entropy.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/entropy_poll.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/gcm.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/hmac_drbg.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/md5.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/poly1305.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/psa_crypto_aead.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/psa_crypto_cipher.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/psa_crypto_ecp.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/psa_crypto_ffdh.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/psa_crypto_hash.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/psa_crypto_mac.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/psa_crypto_pake.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/psa_crypto_rsa.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/psa_crypto_xof.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/psa_util_internal.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/ripemd160.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/rsa.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/rsa_alt_helpers.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/sha1.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/sha256.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/sha3.c",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/src/sha512.c",
        "thirdparty/mbedtls/tf-psa-crypto/extras/lmots.c",
        "thirdparty/mbedtls/tf-psa-crypto/extras/lms.c",
        "thirdparty/mbedtls/tf-psa-crypto/extras/md.c",
        "thirdparty/mbedtls/tf-psa-crypto/extras/nist_kw.c",
        "thirdparty/mbedtls/tf-psa-crypto/extras/pk.c",
        "thirdparty/mbedtls/tf-psa-crypto/extras/pk_ecc.c",
        "thirdparty/mbedtls/tf-psa-crypto/extras/pk_rsa.c",
        "thirdparty/mbedtls/tf-psa-crypto/extras/pk_wrap.c",
        "thirdparty/mbedtls/tf-psa-crypto/extras/pkparse.c",
        "thirdparty/mbedtls/tf-psa-crypto/extras/pkwrite.c",
        "thirdparty/mbedtls/tf-psa-crypto/platform/memory_buffer_alloc.c",
        "thirdparty/mbedtls/tf-psa-crypto/platform/platform.c",
        "thirdparty/mbedtls/tf-psa-crypto/platform/platform_util.c",
        "thirdparty/mbedtls/tf-psa-crypto/platform/threading.c",
        "thirdparty/mbedtls/tf-psa-crypto/utilities/asn1parse.c",
        "thirdparty/mbedtls/tf-psa-crypto/utilities/asn1write.c",
        "thirdparty/mbedtls/tf-psa-crypto/utilities/base64.c",
        "thirdparty/mbedtls/tf-psa-crypto/utilities/constant_time.c",
        "thirdparty/mbedtls/tf-psa-crypto/utilities/oid.c",
        "thirdparty/mbedtls/tf-psa-crypto/utilities/pem.c",
        "thirdparty/mbedtls/tf-psa-crypto/utilities/pkcs5.c",
    }
register_tp("tp_mbedtls")

-- 19. Jolt Physics 3D
if not disable_phys_3d then
    project "tp_jolt_physics"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/jolt_physics" }
        defines { "GODOT_MODULE" }
        if is_dev_build then
            defines { "JPH_ENABLE_ASSERTS" }
        end
        if _OPTIONS["precision"] == "double" then
            defines { "JPH_DOUBLE_PRECISION" }
        end
        if target_platform == "web" and opt_bool("wasm-simd") then
            buildoptions { "-msse4.2" }
        end
        filter "configurations:editor"
            defines { "JPH_DEBUG_RENDERER" }
        filter {}
        files {
            "thirdparty/jolt_physics/Jolt/AABBTree/AABBTreeBuilder.cpp",
            "thirdparty/jolt_physics/Jolt/Core/Color.cpp",
            "thirdparty/jolt_physics/Jolt/Core/Factory.cpp",
            "thirdparty/jolt_physics/Jolt/Core/IssueReporting.cpp",
            "thirdparty/jolt_physics/Jolt/Core/JobSystemSingleThreaded.cpp",
            "thirdparty/jolt_physics/Jolt/Core/JobSystemThreadPool.cpp",
            "thirdparty/jolt_physics/Jolt/Core/JobSystemWithBarrier.cpp",
            "thirdparty/jolt_physics/Jolt/Core/LinearCurve.cpp",
            "thirdparty/jolt_physics/Jolt/Core/Memory.cpp",
            "thirdparty/jolt_physics/Jolt/Core/Profiler.cpp",
            "thirdparty/jolt_physics/Jolt/Core/RTTI.cpp",
            "thirdparty/jolt_physics/Jolt/Core/Semaphore.cpp",
            "thirdparty/jolt_physics/Jolt/Core/StringTools.cpp",
            "thirdparty/jolt_physics/Jolt/Core/TickCounter.cpp",
            "thirdparty/jolt_physics/Jolt/Geometry/ConvexHullBuilder.cpp",
            "thirdparty/jolt_physics/Jolt/Geometry/ConvexHullBuilder2D.cpp",
            "thirdparty/jolt_physics/Jolt/Geometry/Indexify.cpp",
            "thirdparty/jolt_physics/Jolt/Geometry/OrientedBox.cpp",
            "thirdparty/jolt_physics/Jolt/Math/Vec3.cpp",
            "thirdparty/jolt_physics/Jolt/ObjectStream/SerializableObject.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Body/Body.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Body/BodyCreationSettings.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Body/BodyInterface.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Body/BodyManager.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Body/MassProperties.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Body/MotionProperties.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Character/Character.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Character/CharacterBase.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Character/CharacterVirtual.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/BroadPhase/BroadPhase.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/BroadPhase/BroadPhaseBruteForce.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/BroadPhase/BroadPhaseQuadTree.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/BroadPhase/QuadTree.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/CastConvexVsTriangles.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/CastSphereVsTriangles.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/CollideConvexVsTriangles.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/CollideSphereVsTriangles.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/CollisionDispatch.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/CollisionGroup.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/EstimateCollisionResponse.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/GroupFilter.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/GroupFilterTable.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/ManifoldBetweenTwoFaces.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/NarrowPhaseQuery.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/NarrowPhaseStats.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/PhysicsMaterial.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/PhysicsMaterialSimple.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/BoxShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/CapsuleShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/CompoundShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/ConvexHullShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/ConvexShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/CylinderShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/DecoratedShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/EmptyShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/HeightFieldShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/MeshShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/MutableCompoundShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/PlaneShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/RotatedTranslatedShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/ScaledShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/Shape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/SphereShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/StaticCompoundShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/TaperedCapsuleShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/TaperedCylinderShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/Shape/TriangleShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Collision/TransformedShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/ConeConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/Constraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/ConstraintManager.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/ContactConstraintManager.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/DistanceConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/FixedConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/GearConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/HingeConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/MotorSettings.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/PathConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/PathConstraintPath.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/PathConstraintPathHermite.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/PointConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/PulleyConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/RackAndPinionConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/SixDOFConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/SliderConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/SpringSettings.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/SwingTwistConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Constraints/TwoBodyConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/DeterminismLog.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/IslandBuilder.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/LargeIslandSplitter.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/PhysicsScene.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/PhysicsSystem.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/PhysicsUpdateContext.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Ragdoll/Ragdoll.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/SoftBody/SoftBodyCreationSettings.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/SoftBody/SoftBodyMotionProperties.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/SoftBody/SoftBodyShape.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/SoftBody/SoftBodySharedSettings.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/StateRecorderImpl.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Vehicle/MotorcycleController.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Vehicle/TrackedVehicleController.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Vehicle/VehicleAntiRollBar.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Vehicle/VehicleCollisionTester.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Vehicle/VehicleConstraint.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Vehicle/VehicleController.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Vehicle/VehicleDifferential.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Vehicle/VehicleEngine.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Vehicle/VehicleTrack.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Vehicle/VehicleTransmission.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Vehicle/Wheel.cpp",
            "thirdparty/jolt_physics/Jolt/Physics/Vehicle/WheeledVehicleController.cpp",
            "thirdparty/jolt_physics/Jolt/RegisterTypes.cpp",
            "thirdparty/jolt_physics/Jolt/Renderer/DebugRenderer.cpp",
            "thirdparty/jolt_physics/Jolt/Renderer/DebugRendererPlayback.cpp",
            "thirdparty/jolt_physics/Jolt/Renderer/DebugRendererRecorder.cpp",
            "thirdparty/jolt_physics/Jolt/Renderer/DebugRendererSimple.cpp",
            "thirdparty/jolt_physics/Jolt/Skeleton/SkeletalAnimation.cpp",
            "thirdparty/jolt_physics/Jolt/Skeleton/Skeleton.cpp",
            "thirdparty/jolt_physics/Jolt/Skeleton/SkeletonMapper.cpp",
            "thirdparty/jolt_physics/Jolt/Skeleton/SkeletonPose.cpp",
            "thirdparty/jolt_physics/Jolt/TriangleSplitter/TriangleSplitter.cpp",
            "thirdparty/jolt_physics/Jolt/TriangleSplitter/TriangleSplitterBinning.cpp",
            "thirdparty/jolt_physics/Jolt/TriangleSplitter/TriangleSplitterMean.cpp",
        }
    register_tp("tp_jolt_physics")
end

-- 20. Khronos glslang (built when Vulkan, D3D12, or Metal is enabled)
if use_rd then
    project "tp_glslang"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs {
            "thirdparty/glslang",
            "thirdparty",
            "thirdparty/spirv-headers/include",
            "thirdparty/spirv-headers/include/spirv/unified1",
        }
        defines { "GODOT_MODULE", "ENABLE_OPT=0" }
        files {
            "thirdparty/glslang/SPIRV/GlslangToSpv.cpp",
            "thirdparty/glslang/SPIRV/InReadableOrder.cpp",
            "thirdparty/glslang/SPIRV/Logger.cpp",
            "thirdparty/glslang/SPIRV/SpvBuilder.cpp",
            "thirdparty/glslang/SPIRV/SpvPostProcess.cpp",
            "thirdparty/glslang/SPIRV/SpvTools.cpp",
            "thirdparty/glslang/SPIRV/disassemble.cpp",
            "thirdparty/glslang/SPIRV/doc.cpp",
            "thirdparty/glslang/glslang/GenericCodeGen/CodeGen.cpp",
            "thirdparty/glslang/glslang/GenericCodeGen/Link.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/Constant.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/InfoSink.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/Initialize.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/IntermTraverse.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/Intermediate.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/ParseContextBase.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/ParseHelper.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/PoolAlloc.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/RemoveTree.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/Scan.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/ShaderLang.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/SpirvIntrinsics.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/SymbolTable.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/Versions.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/attribute.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/glslang_tab.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/intermOut.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/iomapper.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/limits.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/linkValidate.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/parseConst.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/preprocessor/Pp.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/preprocessor/PpAtom.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/preprocessor/PpContext.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/preprocessor/PpScanner.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/preprocessor/PpTokens.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/propagateNoContraction.cpp",
            "thirdparty/glslang/glslang/MachineIndependent/reflection.cpp",
            "thirdparty/glslang/glslang/ResourceLimits/ResourceLimits.cpp",
        }
        if target_platform == "windows" then
            files { "thirdparty/glslang/glslang/OSDependent/Windows/ossource.cpp" }
        else
            files { "thirdparty/glslang/glslang/OSDependent/Unix/ossource.cpp" }
        end
    register_tp("tp_glslang")
end

-- 21-22. SPIRV-Cross + metal-cpp & Metal driver (Apple arm64 only; requires exceptions & C++20/ObjC++ modules)
if use_metal then
    project "tp_spirv_cross"
        kind "StaticLib"
        apply_common_settings({ exceptions = true })
        warnings "Off"
        includedirs {
            "thirdparty/spirv-cross",
            "thirdparty/spirv-cross/include",
            "thirdparty/spirv-headers/include/spirv/unified1",
            "thirdparty/metal-cpp",
        }
        filter "platforms:arm64"
            files {
                "thirdparty/spirv-cross/spirv_cfg.cpp",
                "thirdparty/spirv-cross/spirv_cross.cpp",
                "thirdparty/spirv-cross/spirv_cross_parsed_ir.cpp",
                "thirdparty/spirv-cross/spirv_cross_util.cpp",
                "thirdparty/spirv-cross/spirv_glsl.cpp",
                "thirdparty/spirv-cross/spirv_msl.cpp",
                "thirdparty/spirv-cross/spirv_parser.cpp",
                "thirdparty/spirv-cross/spirv_reflect.cpp",
                "thirdparty/metal-cpp/metal_cpp.cpp",
            }
        filter {}
    register_tp("tp_spirv_cross")

    project "tp_metal"
        kind "StaticLib"
        apply_common_settings({ cpp20 = true, exceptions = true })
        apply_engine_warnings()
        includedirs {
            "thirdparty/spirv-cross",
            "thirdparty/spirv-cross/include",
            "thirdparty/spirv-headers/include",
            "thirdparty/spirv-headers/include/spirv/unified1",
            "thirdparty/metal-cpp",
            "thirdparty/amd-fsr2",
            "thirdparty/zlib",
            "thirdparty/zstd",
        }
        buildoptions { "-fmodules", "-fcxx-modules" }
        filter "platforms:arm64"
            files {
                "drivers/metal/metal3_objects.cpp",
                "drivers/metal/metal_device_profile.cpp",
                "drivers/metal/metal_device_properties.cpp",
                "drivers/metal/metal_objects_shared.cpp",
                "drivers/metal/pixel_formats.cpp",
                "drivers/metal/rendering_context_driver_metal.cpp",
                "drivers/metal/rendering_device_driver_metal.cpp",
                "drivers/metal/rendering_device_driver_metal3.cpp",
                "drivers/metal/rendering_shader_container_metal.cpp",
            }
        filter {}
    register_tp("tp_metal")
end

-- 23. D3D12 Memory Allocator (Windows D3D12)
if use_d3d12 then
    project "tp_d3d12ma"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        apply_d3d12_mesa_settings()
        files { "thirdparty/d3d12ma/D3D12MemAlloc.cpp" }
    register_tp("tp_d3d12ma")
end

-- 24. Intel Embree raycasting kernels (forced -O3 even in debug/editor builds)
if has_raycast_any then
    project "tp_embree"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        optimize "Speed"
        includedirs { "thirdparty/embree", "thirdparty/embree/include" }
        defines {
            "GODOT_MODULE",
            "EMBREE_TARGET_SSE2",
            "EMBREE_LOWEST_ISA",
            "TASKING_INTERNAL",
            "NDEBUG",
        }
        if is_msvc then
            buildoptions { "/wd4324" }
            defines { "__SSE__", "__SSE2__" }
            filter "platforms:x86_64"
                defines { "__SSE3__", "__SSSE3__", "__SSE4_1__", "__SSE4_2__" }
            filter {}
        else
            filter "platforms:not x86_64"
                defines { "__SSE__", "__SSE2__" }
            filter {}
            if target_platform == "linuxbsd" and not use_llvm then
                filter "platforms:arm64"
                    buildoptions { "-flax-vector-conversions" }
                filter {}
            end
            buildoptions {
                "-fno-strict-overflow",
                "-fno-delete-null-pointer-checks",
                "-fwrapv",
                "-fsigned-char",
                "-fno-strict-aliasing",
                "-fno-tree-vectorize",
                "-fvisibility=hidden",
                "-fvisibility-inlines-hidden",
            }
        end
        for _, a in ipairs(active_arches) do
            if arch_supports_raycast(a) then
                filter("platforms:" .. a)
                    files {
                        "thirdparty/embree/common/lexers/stringstream.cpp",
                        "thirdparty/embree/common/lexers/tokenstream.cpp",
                        "thirdparty/embree/common/math/constants.cpp",
                        "thirdparty/embree/common/simd/sse.cpp",
                        "thirdparty/embree/common/sys/alloc.cpp",
                        "thirdparty/embree/common/sys/barrier.cpp",
                        "thirdparty/embree/common/sys/condition.cpp",
                        "thirdparty/embree/common/sys/estring.cpp",
                        "thirdparty/embree/common/sys/filename.cpp",
                        "thirdparty/embree/common/sys/library.cpp",
                        "thirdparty/embree/common/sys/mutex.cpp",
                        "thirdparty/embree/common/sys/regression.cpp",
                        "thirdparty/embree/common/sys/sysinfo.cpp",
                        "thirdparty/embree/common/sys/thread.cpp",
                        "thirdparty/embree/common/tasking/taskschedulerinternal.cpp",
                        "thirdparty/embree/kernels/builders/primrefgen.cpp",
                        "thirdparty/embree/kernels/bvh/bvh.cpp",
                        "thirdparty/embree/kernels/bvh/bvh4_factory.cpp",
                        "thirdparty/embree/kernels/bvh/bvh8_factory.cpp",
                        "thirdparty/embree/kernels/bvh/bvh_builder.cpp",
                        "thirdparty/embree/kernels/bvh/bvh_builder_morton.cpp",
                        "thirdparty/embree/kernels/bvh/bvh_builder_sah.cpp",
                        "thirdparty/embree/kernels/bvh/bvh_builder_sah_mb.cpp",
                        "thirdparty/embree/kernels/bvh/bvh_builder_sah_spatial.cpp",
                        "thirdparty/embree/kernels/bvh/bvh_builder_twolevel.cpp",
                        "thirdparty/embree/kernels/bvh/bvh_collider.cpp",
                        "thirdparty/embree/kernels/bvh/bvh_intersector1_bvh4.cpp",
                        "thirdparty/embree/kernels/bvh/bvh_intersector_hybrid4_bvh4.cpp",
                        "thirdparty/embree/kernels/bvh/bvh_refit.cpp",
                        "thirdparty/embree/kernels/bvh/bvh_rotate.cpp",
                        "thirdparty/embree/kernels/bvh/bvh_statistics.cpp",
                        "thirdparty/embree/kernels/common/acceln.cpp",
                        "thirdparty/embree/kernels/common/accelset.cpp",
                        "thirdparty/embree/kernels/common/alloc.cpp",
                        "thirdparty/embree/kernels/common/device.cpp",
                        "thirdparty/embree/kernels/common/geometry.cpp",
                        "thirdparty/embree/kernels/common/rtcore.cpp",
                        "thirdparty/embree/kernels/common/rtcore_builder.cpp",
                        "thirdparty/embree/kernels/common/scene.cpp",
                        "thirdparty/embree/kernels/common/scene_triangle_mesh.cpp",
                        "thirdparty/embree/kernels/common/scene_verify.cpp",
                        "thirdparty/embree/kernels/common/stat.cpp",
                        "thirdparty/embree/kernels/common/state.cpp",
                        "thirdparty/embree/kernels/geometry/primitive4.cpp",
                    }
                filter {}
            end
        end
    register_tp("tp_embree")
end

-- 25. SDL3 input/gamepad/haptic backend
if use_sdl then
    project "tp_sdl"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs {
            "drivers/sdl",
            "thirdparty/sdl",
            "thirdparty/sdl/include",
            "thirdparty/sdl/include/build_config",
        }
        defines { "SDL_PLATFORM_PRIVATE" }
        files {
            "thirdparty/sdl/SDL.c",
            "thirdparty/sdl/SDL_assert.c",
            "thirdparty/sdl/SDL_error.c",
            "thirdparty/sdl/SDL_guid.c",
            "thirdparty/sdl/SDL_hashtable.c",
            "thirdparty/sdl/SDL_hints.c",
            "thirdparty/sdl/SDL_list.c",
            "thirdparty/sdl/SDL_log.c",
            "thirdparty/sdl/SDL_properties.c",
            "thirdparty/sdl/SDL_utils.c",
            "thirdparty/sdl/atomic/SDL_atomic.c",
            "thirdparty/sdl/atomic/SDL_spinlock.c",
            "thirdparty/sdl/events/SDL_events.c",
            "thirdparty/sdl/events/SDL_eventwatch.c",
            "thirdparty/sdl/haptic/SDL_haptic.c",
            "thirdparty/sdl/hidapi/SDL_hidapi.c",
            "thirdparty/sdl/io/SDL_iostream.c",
            "thirdparty/sdl/joystick/SDL_gamepad.c",
            "thirdparty/sdl/joystick/SDL_joystick.c",
            "thirdparty/sdl/joystick/SDL_steam_virtual_gamepad.c",
            "thirdparty/sdl/joystick/controller_type.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_combined.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_gamecube.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_luna.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_ps3.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_ps4.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_ps5.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_rumble.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_shield.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_stadia.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_steam.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_steam_hori.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_steamdeck.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_switch.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_wii.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_xbox360.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_xbox360w.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapi_xboxone.c",
            "thirdparty/sdl/joystick/hidapi/SDL_hidapijoystick.c",
            "thirdparty/sdl/libm/e_atan2.c",
            "thirdparty/sdl/libm/e_exp.c",
            "thirdparty/sdl/libm/e_fmod.c",
            "thirdparty/sdl/libm/e_log.c",
            "thirdparty/sdl/libm/e_log10.c",
            "thirdparty/sdl/libm/e_pow.c",
            "thirdparty/sdl/libm/e_rem_pio2.c",
            "thirdparty/sdl/libm/e_sqrt.c",
            "thirdparty/sdl/libm/k_cos.c",
            "thirdparty/sdl/libm/k_rem_pio2.c",
            "thirdparty/sdl/libm/k_sin.c",
            "thirdparty/sdl/libm/k_tan.c",
            "thirdparty/sdl/libm/s_atan.c",
            "thirdparty/sdl/libm/s_copysign.c",
            "thirdparty/sdl/libm/s_cos.c",
            "thirdparty/sdl/libm/s_fabs.c",
            "thirdparty/sdl/libm/s_floor.c",
            "thirdparty/sdl/libm/s_isinf.c",
            "thirdparty/sdl/libm/s_isinff.c",
            "thirdparty/sdl/libm/s_isnan.c",
            "thirdparty/sdl/libm/s_isnanf.c",
            "thirdparty/sdl/libm/s_modf.c",
            "thirdparty/sdl/libm/s_scalbn.c",
            "thirdparty/sdl/libm/s_sin.c",
            "thirdparty/sdl/libm/s_tan.c",
            "thirdparty/sdl/sensor/SDL_sensor.c",
            "thirdparty/sdl/sensor/dummy/SDL_dummysensor.c",
            "thirdparty/sdl/stdlib/SDL_crc16.c",
            "thirdparty/sdl/stdlib/SDL_crc32.c",
            "thirdparty/sdl/stdlib/SDL_getenv.c",
            "thirdparty/sdl/stdlib/SDL_iconv.c",
            "thirdparty/sdl/stdlib/SDL_malloc.c",
            "thirdparty/sdl/stdlib/SDL_memcpy.c",
            "thirdparty/sdl/stdlib/SDL_memmove.c",
            "thirdparty/sdl/stdlib/SDL_memset.c",
            "thirdparty/sdl/stdlib/SDL_mslibc.c",
            "thirdparty/sdl/stdlib/SDL_murmur3.c",
            "thirdparty/sdl/stdlib/SDL_qsort.c",
            "thirdparty/sdl/stdlib/SDL_random.c",
            "thirdparty/sdl/stdlib/SDL_stdlib.c",
            "thirdparty/sdl/stdlib/SDL_string.c",
            "thirdparty/sdl/stdlib/SDL_strtokr.c",
            "thirdparty/sdl/thread/SDL_thread.c",
            "thirdparty/sdl/timer/SDL_timer.c",
        }
        if target_platform == "linuxbsd" then
            includedirs { "thirdparty/sdl/core/linux" }
            defines { "SDL_PLATFORM_LINUX" }
            files {
                "thirdparty/sdl/core/linux/SDL_dbus.c",
                "thirdparty/sdl/core/linux/SDL_evdev.c",
                "thirdparty/sdl/core/linux/SDL_evdev_capabilities.c",
                "thirdparty/sdl/core/linux/SDL_evdev_kbd.c",
                "thirdparty/sdl/core/linux/SDL_threadprio.c",
                "thirdparty/sdl/core/linux/SDL_udev.c",
                "thirdparty/sdl/core/unix/SDL_appid.c",
                "thirdparty/sdl/core/unix/SDL_poll.c",
                "thirdparty/sdl/haptic/linux/SDL_syshaptic.c",
                "thirdparty/sdl/joystick/linux/SDL_sysjoystick.c",
                "thirdparty/sdl/loadso/dlopen/SDL_sysloadso.c",
                "thirdparty/sdl/thread/pthread/SDL_syscond.c",
                "thirdparty/sdl/thread/pthread/SDL_sysmutex.c",
                "thirdparty/sdl/thread/pthread/SDL_sysrwlock.c",
                "thirdparty/sdl/thread/pthread/SDL_syssem.c",
                "thirdparty/sdl/thread/pthread/SDL_systhread.c",
                "thirdparty/sdl/thread/pthread/SDL_systls.c",
                "thirdparty/sdl/timer/unix/SDL_systimer.c",
            }
        elseif target_platform == "macos" then
            defines { "SDL_PLATFORM_MACOS" }
            files {
                "thirdparty/sdl/core/unix/SDL_appid.c",
                "thirdparty/sdl/core/unix/SDL_poll.c",
                "thirdparty/sdl/haptic/darwin/SDL_syshaptic.c",
                "thirdparty/sdl/joystick/darwin/SDL_iokitjoystick.c",
                "thirdparty/sdl/joystick/apple/SDL_mfijoystick.m",
                "thirdparty/sdl/thread/pthread/SDL_syscond.c",
                "thirdparty/sdl/thread/pthread/SDL_sysmutex.c",
                "thirdparty/sdl/thread/pthread/SDL_sysrwlock.c",
                "thirdparty/sdl/thread/pthread/SDL_syssem.c",
                "thirdparty/sdl/thread/pthread/SDL_systhread.c",
                "thirdparty/sdl/thread/pthread/SDL_systls.c",
                "thirdparty/sdl/timer/unix/SDL_systimer.c",
            }
        elseif target_platform == "windows" then
            defines { "SDL_PLATFORM_WINDOWS" }
            files {
                "thirdparty/sdl/core/windows/SDL_gameinput.c",
                "thirdparty/sdl/core/windows/SDL_hid.c",
                "thirdparty/sdl/core/windows/SDL_immdevice.c",
                "thirdparty/sdl/core/windows/SDL_windows.c",
                "thirdparty/sdl/core/windows/SDL_xinput.c",
                "thirdparty/sdl/core/windows/pch.c",
                "thirdparty/sdl/haptic/windows/SDL_dinputhaptic.c",
                "thirdparty/sdl/haptic/windows/SDL_windowshaptic.c",
                "thirdparty/sdl/joystick/windows/SDL_dinputjoystick.c",
                "thirdparty/sdl/joystick/windows/SDL_rawinputjoystick.c",
                "thirdparty/sdl/joystick/windows/SDL_windows_gaming_input.c",
                "thirdparty/sdl/joystick/windows/SDL_windowsjoystick.c",
                "thirdparty/sdl/joystick/windows/SDL_xinputjoystick.c",
                "thirdparty/sdl/sensor/windows/SDL_windowssensor.c",
                "thirdparty/sdl/thread/generic/SDL_syscond.c",
                "thirdparty/sdl/thread/generic/SDL_sysrwlock.c",
                "thirdparty/sdl/thread/windows/SDL_syscond_cv.c",
                "thirdparty/sdl/thread/windows/SDL_sysmutex.c",
                "thirdparty/sdl/thread/windows/SDL_sysrwlock_srw.c",
                "thirdparty/sdl/thread/windows/SDL_syssem.c",
                "thirdparty/sdl/thread/windows/SDL_systhread.c",
                "thirdparty/sdl/thread/windows/SDL_systls.c",
                "thirdparty/sdl/timer/windows/SDL_systimer.c",
            }
        elseif target_platform == "ios" or target_platform == "visionos" then
            defines { (target_platform == "ios") and "SDL_PLATFORM_IOS" or "SDL_PLATFORM_VISIONOS" }
            files {
                "thirdparty/sdl/core/unix/SDL_appid.c",
                "thirdparty/sdl/core/unix/SDL_poll.c",
                "thirdparty/sdl/haptic/dummy/SDL_syshaptic.c",
                "thirdparty/sdl/joystick/apple/SDL_mfijoystick.m",
                "thirdparty/sdl/thread/pthread/SDL_syscond.c",
                "thirdparty/sdl/thread/pthread/SDL_sysmutex.c",
                "thirdparty/sdl/thread/pthread/SDL_sysrwlock.c",
                "thirdparty/sdl/thread/pthread/SDL_syssem.c",
                "thirdparty/sdl/thread/pthread/SDL_systhread.c",
                "thirdparty/sdl/thread/pthread/SDL_systls.c",
                "thirdparty/sdl/timer/unix/SDL_systimer.c",
            }
        end
    register_tp("tp_sdl")
end

-- 26. Khronos OpenXR loader & bundled jsoncpp
if has_openxr then
    project "tp_openxr"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs {
            "thirdparty/openxr",
            "thirdparty/openxr/include",
            "thirdparty/openxr/src",
            "thirdparty/openxr/src/common",
            "thirdparty/openxr/src/loader",
            "thirdparty/openxr/src/external/jsoncpp/include",
            "thirdparty/vulkan/include",
        }
        defines {
            "GODOT_MODULE",
            "DISABLE_STD_FILESYSTEM",
            "XRLOADER_DISABLE_EXCEPTION_HANDLING",
            "JSON_USE_EXCEPTION=0",
        }
        if target_platform == "linuxbsd" then
            defines { "XR_OS_LINUX" }
            if opt_bool("x11") then defines { "XR_USE_PLATFORM_XLIB" } end
            if opt_bool("wayland") and use_opengl3 then
                includedirs { "thirdparty/glad" }
                defines { "XR_USE_PLATFORM_EGL", "XRDEPENDENCIES_USE_GLAD" }
            end
            if host_os ~= "bsd" then defines { "HAVE_SECURE_GETENV" } end
        elseif target_platform == "windows" then
            defines { "XR_OS_WINDOWS", "XR_USE_PLATFORM_WIN32" }
        elseif target_platform == "macos" then
            defines { "XR_OS_APPLE" }
        elseif target_platform == "android" then
            defines { "XR_OS_ANDROID", "XR_USE_PLATFORM_ANDROID" }
        end
        files {
            "thirdparty/openxr/src/external/jsoncpp/src/lib_json/json_reader.cpp",
            "thirdparty/openxr/src/external/jsoncpp/src/lib_json/json_value.cpp",
            "thirdparty/openxr/src/external/jsoncpp/src/lib_json/json_writer.cpp",
        }
        -- Android links the OpenXR loader via Gradle AAR; all desktop platforms build it from source
        if target_platform ~= "android" then
            files {
                "thirdparty/openxr/src/common/filesystem_utils.cpp",
                "thirdparty/openxr/src/common/object_info.cpp",
                "thirdparty/openxr/src/loader/api_layer_interface.cpp",
                "thirdparty/openxr/src/loader/loader_core.cpp",
                "thirdparty/openxr/src/loader/loader_init_data.cpp",
                "thirdparty/openxr/src/loader/loader_instance.cpp",
                "thirdparty/openxr/src/loader/loader_logger.cpp",
                "thirdparty/openxr/src/loader/loader_logger_recorders.cpp",
                "thirdparty/openxr/src/loader/loader_properties.cpp",
                "thirdparty/openxr/src/loader/manifest_file.cpp",
                "thirdparty/openxr/src/loader/runtime_interface.cpp",
                "thirdparty/openxr/src/xr_generated_dispatch_table_core.c",
            }
        end
    register_tp("tp_openxr")
end

-- 27. Manifold CSG
if not disable_3d then
    project "tp_manifold"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/manifold/include" }
        defines { "GODOT_MODULE", "MANIFOLD_PAR=-1" }
        files {
            "thirdparty/manifold/src/boolean3.cpp",
            "thirdparty/manifold/src/boolean_result.cpp",
            "thirdparty/manifold/src/constructors.cpp",
            "thirdparty/manifold/src/csg_tree.cpp",
            "thirdparty/manifold/src/edge_op.cpp",
            "thirdparty/manifold/src/face_op.cpp",
            "thirdparty/manifold/src/impl.cpp",
            "thirdparty/manifold/src/manifold.cpp",
            "thirdparty/manifold/src/polygon.cpp",
            "thirdparty/manifold/src/properties.cpp",
            "thirdparty/manifold/src/quickhull.cpp",
            "thirdparty/manifold/src/sdf.cpp",
            "thirdparty/manifold/src/smoothing.cpp",
            "thirdparty/manifold/src/sort.cpp",
            "thirdparty/manifold/src/subdivision.cpp",
            "thirdparty/manifold/src/tree2d.cpp",
        }
    register_tp("tp_manifold")
end

-- 28-30. Recast + RVO2-2D + RVO2-3D
if not disable_nav_3d then
    project "tp_recast"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/recastnavigation/Recast/Include" }
        defines { "GODOT_MODULE" }
        files {
            "thirdparty/recastnavigation/Recast/Source/Recast.cpp",
            "thirdparty/recastnavigation/Recast/Source/RecastAlloc.cpp",
            "thirdparty/recastnavigation/Recast/Source/RecastArea.cpp",
            "thirdparty/recastnavigation/Recast/Source/RecastAssert.cpp",
            "thirdparty/recastnavigation/Recast/Source/RecastContour.cpp",
            "thirdparty/recastnavigation/Recast/Source/RecastFilter.cpp",
            "thirdparty/recastnavigation/Recast/Source/RecastLayers.cpp",
            "thirdparty/recastnavigation/Recast/Source/RecastMesh.cpp",
            "thirdparty/recastnavigation/Recast/Source/RecastMeshDetail.cpp",
            "thirdparty/recastnavigation/Recast/Source/RecastRasterization.cpp",
            "thirdparty/recastnavigation/Recast/Source/RecastRegion.cpp",
        }
    register_tp("tp_recast")
end

if not disable_nav_2d or not disable_nav_3d then
    project "tp_rvo2_2d"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/rvo2/rvo2_2d" }
        defines { "GODOT_MODULE" }
        files {
            "thirdparty/rvo2/rvo2_2d/Agent2d.cpp",
            "thirdparty/rvo2/rvo2_2d/KdTree2d.cpp",
            "thirdparty/rvo2/rvo2_2d/Obstacle2d.cpp",
            "thirdparty/rvo2/rvo2_2d/RVOSimulator2d.cpp",
        }
    register_tp("tp_rvo2_2d")
end

if not disable_nav_3d then
    project "tp_rvo2_3d"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/rvo2/rvo2_3d" }
        defines { "GODOT_MODULE" }
        files {
            "thirdparty/rvo2/rvo2_3d/Agent3d.cpp",
            "thirdparty/rvo2/rvo2_3d/KdTree3d.cpp",
            "thirdparty/rvo2/rvo2_3d/RVOSimulator3d.cpp",
        }
    register_tp("tp_rvo2_3d")
end

-- 31. ASTCenc (compressor in editor; decompress-only in export templates)
project "tp_astcenc"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs { "thirdparty/astcenc" }
    defines { "GODOT_MODULE" }
    filter "configurations:template_debug or template_release"
        defines { "ASTCENC_DECOMPRESS_ONLY" }
    filter {}
    files {
        "thirdparty/astcenc/astcenc_averages_and_directions.cpp",
        "thirdparty/astcenc/astcenc_block_sizes.cpp",
        "thirdparty/astcenc/astcenc_color_quantize.cpp",
        "thirdparty/astcenc/astcenc_color_unquantize.cpp",
        "thirdparty/astcenc/astcenc_compress_symbolic.cpp",
        "thirdparty/astcenc/astcenc_compute_variance.cpp",
        "thirdparty/astcenc/astcenc_decompress_symbolic.cpp",
        "thirdparty/astcenc/astcenc_diagnostic_trace.cpp",
        "thirdparty/astcenc/astcenc_entry.cpp",
        "thirdparty/astcenc/astcenc_find_best_partitioning.cpp",
        "thirdparty/astcenc/astcenc_ideal_endpoints_and_weights.cpp",
        "thirdparty/astcenc/astcenc_image.cpp",
        "thirdparty/astcenc/astcenc_integer_sequence.cpp",
        "thirdparty/astcenc/astcenc_mathlib.cpp",
        "thirdparty/astcenc/astcenc_mathlib_softfloat.cpp",
        "thirdparty/astcenc/astcenc_partition_tables.cpp",
        "thirdparty/astcenc/astcenc_percentile_tables.cpp",
        "thirdparty/astcenc/astcenc_pick_best_endpoint_format.cpp",
        "thirdparty/astcenc/astcenc_quantization.cpp",
        "thirdparty/astcenc/astcenc_symbolic_physical.cpp",
        "thirdparty/astcenc/astcenc_weight_align.cpp",
        "thirdparty/astcenc/astcenc_weight_quant_xfer_tables.cpp",
    }
register_tp("tp_astcenc")

-- 32. CVTT (editor or --cvtt-export-templates=yes)
project "tp_cvtt"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs { "thirdparty/cvtt" }
    defines { "GODOT_MODULE" }
    if cvtt_templates then
        files {
            "thirdparty/cvtt/ConvectionKernels_API.cpp",
            "thirdparty/cvtt/ConvectionKernels_BC67.cpp",
            "thirdparty/cvtt/ConvectionKernels_BC6H_IO.cpp",
            "thirdparty/cvtt/ConvectionKernels_BC7_PrioData.cpp",
            "thirdparty/cvtt/ConvectionKernels_BCCommon.cpp",
            "thirdparty/cvtt/ConvectionKernels_ETC.cpp",
            "thirdparty/cvtt/ConvectionKernels_IndexSelector.cpp",
            "thirdparty/cvtt/ConvectionKernels_S3TC.cpp",
            "thirdparty/cvtt/ConvectionKernels_SingleFile.cpp",
            "thirdparty/cvtt/ConvectionKernels_Util.cpp",
        }
    else
        filter "configurations:editor"
            files {
                "thirdparty/cvtt/ConvectionKernels_API.cpp",
                "thirdparty/cvtt/ConvectionKernels_BC67.cpp",
                "thirdparty/cvtt/ConvectionKernels_BC6H_IO.cpp",
                "thirdparty/cvtt/ConvectionKernels_BC7_PrioData.cpp",
                "thirdparty/cvtt/ConvectionKernels_BCCommon.cpp",
                "thirdparty/cvtt/ConvectionKernels_ETC.cpp",
                "thirdparty/cvtt/ConvectionKernels_IndexSelector.cpp",
                "thirdparty/cvtt/ConvectionKernels_S3TC.cpp",
                "thirdparty/cvtt/ConvectionKernels_SingleFile.cpp",
                "thirdparty/cvtt/ConvectionKernels_Util.cpp",
            }
        filter {}
    end
register_tp("tp_cvtt", cvtt_templates and "always" or "cvtt")

-- 33. etcpak
project "tp_etcpak"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs { "thirdparty/etcpak" }
    defines { "GODOT_MODULE" }
    files {
        "thirdparty/etcpak/DecodeRGB.cpp",
        "thirdparty/etcpak/Dither.cpp",
        "thirdparty/etcpak/ProcessDxtc.cpp",
        "thirdparty/etcpak/ProcessRGB.cpp",
        "thirdparty/etcpak/Tables.cpp",
    }
register_tp("tp_etcpak")

-- 34. ENet
project "tp_enet"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs {
        "thirdparty/enet",
        "thirdparty/mbedtls/godot",
        "thirdparty/mbedtls/include",
        "thirdparty/mbedtls/tf-psa-crypto/include",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/include",
    }
    defines { "GODOT_MODULE", "GODOT_ENET" }
    files {
        "thirdparty/enet/callbacks.c",
        "thirdparty/enet/compress.c",
        "thirdparty/enet/enet_godot.cpp",
        "thirdparty/enet/host.c",
        "thirdparty/enet/list.c",
        "thirdparty/enet/packet.c",
        "thirdparty/enet/peer.c",
        "thirdparty/enet/protocol.c",
    }
register_tp("tp_enet")

-- 35. meshoptimizer
if not disable_3d then
    project "tp_meshoptimizer"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/meshoptimizer" }
        defines { "GODOT_MODULE" }
        files {
            "thirdparty/meshoptimizer/allocator.cpp",
            "thirdparty/meshoptimizer/clusterizer.cpp",
            "thirdparty/meshoptimizer/indexanalyzer.cpp",
            "thirdparty/meshoptimizer/indexcodec.cpp",
            "thirdparty/meshoptimizer/indexgenerator.cpp",
            "thirdparty/meshoptimizer/overdrawoptimizer.cpp",
            "thirdparty/meshoptimizer/partition.cpp",
            "thirdparty/meshoptimizer/quantization.cpp",
            "thirdparty/meshoptimizer/rasterizer.cpp",
            "thirdparty/meshoptimizer/simplifier.cpp",
            "thirdparty/meshoptimizer/spatialorder.cpp",
            "thirdparty/meshoptimizer/stripifier.cpp",
            "thirdparty/meshoptimizer/tangentspace.cpp",
            "thirdparty/meshoptimizer/vcacheoptimizer.cpp",
            "thirdparty/meshoptimizer/vertexcodec.cpp",
            "thirdparty/meshoptimizer/vertexfilter.cpp",
            "thirdparty/meshoptimizer/vfetchoptimizer.cpp",
        }
    register_tp("tp_meshoptimizer")
end

-- 36. MiniUPnPc (omitted on Web)
if target_platform ~= "web" then
    project "tp_miniupnpc"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs {
            "thirdparty/miniupnpc/include",
            "thirdparty/miniupnpc/include/miniupnpc",
        }
        defines { "GODOT_MODULE", "MINIUPNP_STATICLIB" }
        if target_platform ~= "windows" then
            defines { "MINIUPNPC_SET_SOCKET_TIMEOUT" }
        end
        files {
            "thirdparty/miniupnpc/src/addr_is_reserved.c",
            "thirdparty/miniupnpc/src/connecthostport.c",
            "thirdparty/miniupnpc/src/igd_desc_parse.c",
            "thirdparty/miniupnpc/src/minisoap.c",
            "thirdparty/miniupnpc/src/minissdpc.c",
            "thirdparty/miniupnpc/src/miniupnpc.c",
            "thirdparty/miniupnpc/src/miniwget.c",
            "thirdparty/miniupnpc/src/minixml.c",
            "thirdparty/miniupnpc/src/portlistingparse.c",
            "thirdparty/miniupnpc/src/receivedata.c",
            "thirdparty/miniupnpc/src/upnpcommands.c",
            "thirdparty/miniupnpc/src/upnpdev.c",
            "thirdparty/miniupnpc/src/upnpreplyparse.c",
        }
    register_tp("tp_miniupnpc")
end

-- 37. ufbx
if not disable_3d then
    project "tp_ufbx"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/ufbx" }
        defines {
            "GODOT_MODULE",
            "UFBX_NO_SUBDIVISION",
            "UFBX_NO_TESSELLATION",
            "UFBX_NO_GEOMETRY_CACHE",
            "UFBX_NO_SCENE_EVALUATION",
            "UFBX_NO_INDEX_GENERATION",
            "UFBX_NO_SKINNING_EVALUATION",
            "UFBX_NO_FORMAT_OBJ",
        }
        files {
            "thirdparty/ufbx/ufbx.c",
        }
    register_tp("tp_ufbx")
end

-- 38. V-HACD
if not disable_phys_3d then
    project "tp_vhacd"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/vhacd/inc" }
        defines { "GODOT_MODULE" }
        files {
            "thirdparty/vhacd/src/FloatMath.cpp",
            "thirdparty/vhacd/src/VHACD-ASYNC.cpp",
            "thirdparty/vhacd/src/VHACD.cpp",
            "thirdparty/vhacd/src/btAlignedAllocator.cpp",
            "thirdparty/vhacd/src/btConvexHullComputer.cpp",
            "thirdparty/vhacd/src/vhacdICHull.cpp",
            "thirdparty/vhacd/src/vhacdManifoldMesh.cpp",
            "thirdparty/vhacd/src/vhacdMesh.cpp",
            "thirdparty/vhacd/src/vhacdRaycastMesh.cpp",
            "thirdparty/vhacd/src/vhacdVolume.cpp",
        }
    register_tp("tp_vhacd")
end

-- 39. Wslay (omitted on Web, which uses browser WebSocket JS library)
if target_platform ~= "web" then
    project "tp_wslay"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/wslay" }
        defines { "GODOT_MODULE", "HAVE_CONFIG_H" }
        if target_platform == "windows" then
            defines { "HAVE_WINSOCK2_H" }
        else
            defines { "HAVE_NETINET_IN_H" }
        end
        files {
            "thirdparty/wslay/wslay_event.c",
            "thirdparty/wslay/wslay_frame.c",
            "thirdparty/wslay/wslay_net.c",
            "thirdparty/wslay/wslay_queue.c",
        }
    register_tp("tp_wslay")
end

-- 40. xatlas (editor only)
if target_platform ~= "ios" and target_platform ~= "visionos" then
    project "tp_xatlas"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/xatlas" }
        defines { "GODOT_MODULE" }
        filter "configurations:editor"
            files {
                "thirdparty/xatlas/xatlas.cpp",
            }
        filter {}
    register_tp("tp_xatlas", "editor")
end

-- 41. Vulkan Memory Allocator
local function apply_vulkan_platform_defines()
    if target_platform == "linuxbsd" then
        if opt_bool("x11") then defines { "VK_USE_PLATFORM_XLIB_KHR" } end
        if opt_bool("wayland") then defines { "VK_USE_PLATFORM_WAYLAND_KHR" } end
    elseif target_platform == "windows" then
        defines { "VK_USE_PLATFORM_WIN32_KHR" }
    elseif target_platform == "macos" then
        defines { "VK_USE_PLATFORM_MACOS_MVK", "VK_USE_PLATFORM_METAL_EXT" }
    elseif target_platform == "ios" then
        defines { "VK_USE_PLATFORM_IOS_MVK", "VK_USE_PLATFORM_METAL_EXT" }
    elseif target_platform == "android" then
        defines { "VK_USE_PLATFORM_ANDROID_KHR" }
    end
end

if use_vulkan then
    project "tp_vma"
        kind "StaticLib"
        apply_common_settings()
        warnings "Off"
        includedirs { "thirdparty/vulkan", "thirdparty/vulkan/include" }
        defines { "VMA_EXTERNAL_MEMORY_WIN32=0" }
        if use_volk then
            includedirs { "thirdparty/volk" }
            defines { "USE_VOLK", "VMA_STATIC_VULKAN_FUNCTIONS=1" }
        elseif target_platform == "android" then
            defines { "VMA_VULKAN_VERSION=1000000" }
        elseif target_platform == "macos" or target_platform == "ios" then
            defines { "VMA_VULKAN_VERSION=1001000" }
        end
        apply_vulkan_platform_defines()
        files { "thirdparty/vulkan/vk_mem_alloc.cpp" }
    register_tp("tp_vma")
end

-- 42-44. Windows-specific isolated TUs (libbacktrace, non-SSE4.2 CPU check, C++20 WinRT TTS)
if target_platform == "windows" then
    if not is_msvc then
        project "tp_libbacktrace"
            kind "StaticLib"
            apply_common_settings()
            warnings "Off"
            includedirs { "thirdparty/libbacktrace" }
            files {
                "thirdparty/libbacktrace/alloc.c",
                "thirdparty/libbacktrace/atomic.c",
                "thirdparty/libbacktrace/backtrace.c",
                "thirdparty/libbacktrace/dwarf.c",
                "thirdparty/libbacktrace/fileline.c",
                "thirdparty/libbacktrace/pecoff.c",
                "thirdparty/libbacktrace/posix.c",
                "thirdparty/libbacktrace/print.c",
                "thirdparty/libbacktrace/read.c",
                "thirdparty/libbacktrace/simple.c",
                "thirdparty/libbacktrace/sort.c",
                "thirdparty/libbacktrace/state.c",
            }
        register_tp("tp_libbacktrace")
    end

    if library_type == "executable" then
        project "tp_win_cpucheck"
            kind "StaticLib"
            apply_common_settings({ no_sse42 = true })
            warnings "Off"
            filter "platforms:x86_64"
                files { "platform/windows/cpu_feature_validation.c" }
            filter {}
        register_tp("tp_win_cpucheck")
    end

    project "tp_winrt"
        kind "StaticLib"
        apply_common_settings({ cpp20 = use_winrt, exceptions = use_winrt })
        apply_engine_warnings()
        includedirs {
            "thirdparty/zlib",
            "thirdparty/zstd",
        }
        files {
            "platform/windows/tts_windows.cpp",
            "platform/windows/tts_driver_sapi.cpp",
            "platform/windows/winrt_utils.cpp",
        }
        if use_winrt then
            defines { "WINRT_ENABLED" }
            if not is_msvc and os.isdir(winrt_mingw_path) then
                includedirs { winrt_mingw_path }
            end
            files { "platform/windows/tts_driver_onecore.cpp" }
        end
    register_tp("tp_winrt")
end

-- 45. Core & low-level rendering thirdparty archive (zlib, zstd, brotli, libpng, clipper2, minizip, misc, fsr2, spirv-reflect, glad, volk, re-spirv)
project "tp_core"
    kind "StaticLib"
    apply_common_settings()
    warnings "Off"
    includedirs {
        "thirdparty/amd-fsr2",
        "thirdparty/clipper2/include",
        "thirdparty/libpng",
        "thirdparty/minizip",
        "thirdparty/spirv-headers/include",
        "thirdparty/zlib",
        "thirdparty/zstd",
        "thirdparty/zstd/common",
    }
    defines {
        "CLIPPER2_ENABLED",
        "ZSTD_STATIC_LINKING_ONLY",
        "ZSTD_DISABLE_ASM",
    }
    if not is_msvc then
        defines { "FFX_GCC" }
    end
    files {
        "thirdparty/amd-fsr2/ffx_assert.cpp",
        "thirdparty/amd-fsr2/ffx_fsr2.cpp",
        "thirdparty/clipper2/src/clipper.engine.cpp",
        "thirdparty/clipper2/src/clipper.offset.cpp",
        "thirdparty/clipper2/src/clipper.rectclip.cpp",
        "thirdparty/minizip/ioapi.c",
        "thirdparty/minizip/unzip.c",
        "thirdparty/minizip/zip.c",
        "thirdparty/misc/fastlz.c",
        "thirdparty/misc/pcg.cpp",
        "thirdparty/misc/polypartition.cpp",
        "thirdparty/misc/qoa.c",
        "thirdparty/misc/r128.c",
        "thirdparty/misc/smaz.c",
        "thirdparty/misc/smolv.cpp",
        "thirdparty/spirv-reflect/spirv_reflect.c",
        "thirdparty/zlib/adler32.c",
        "thirdparty/zlib/compress.c",
        "thirdparty/zlib/crc32.c",
        "thirdparty/zlib/deflate.c",
        "thirdparty/zlib/inffast.c",
        "thirdparty/zlib/inflate.c",
        "thirdparty/zlib/inftrees.c",
        "thirdparty/zlib/trees.c",
        "thirdparty/zlib/uncompr.c",
        "thirdparty/zlib/zutil.c",
        "thirdparty/zstd/common/debug.c",
        "thirdparty/zstd/common/entropy_common.c",
        "thirdparty/zstd/common/error_private.c",
        "thirdparty/zstd/common/fse_decompress.c",
        "thirdparty/zstd/common/pool.c",
        "thirdparty/zstd/common/threading.c",
        "thirdparty/zstd/common/xxhash.c",
        "thirdparty/zstd/common/zstd_common.c",
        "thirdparty/zstd/compress/fse_compress.c",
        "thirdparty/zstd/compress/hist.c",
        "thirdparty/zstd/compress/huf_compress.c",
        "thirdparty/zstd/compress/zstd_compress.c",
        "thirdparty/zstd/compress/zstd_compress_literals.c",
        "thirdparty/zstd/compress/zstd_compress_sequences.c",
        "thirdparty/zstd/compress/zstd_compress_superblock.c",
        "thirdparty/zstd/compress/zstd_double_fast.c",
        "thirdparty/zstd/compress/zstd_fast.c",
        "thirdparty/zstd/compress/zstd_lazy.c",
        "thirdparty/zstd/compress/zstd_ldm.c",
        "thirdparty/zstd/compress/zstd_opt.c",
        "thirdparty/zstd/compress/zstd_preSplit.c",
        "thirdparty/zstd/compress/zstdmt_compress.c",
        "thirdparty/zstd/decompress/huf_decompress.c",
        "thirdparty/zstd/decompress/zstd_ddict.c",
        "thirdparty/zstd/decompress/zstd_decompress.c",
        "thirdparty/zstd/decompress/zstd_decompress_block.c",
        "thirdparty/libpng/png.c",
        "thirdparty/libpng/pngerror.c",
        "thirdparty/libpng/pngget.c",
        "thirdparty/libpng/pngmem.c",
        "thirdparty/libpng/pngpread.c",
        "thirdparty/libpng/pngread.c",
        "thirdparty/libpng/pngrio.c",
        "thirdparty/libpng/pngrtran.c",
        "thirdparty/libpng/pngrutil.c",
        "thirdparty/libpng/pngset.c",
        "thirdparty/libpng/pngtrans.c",
        "thirdparty/libpng/pngwio.c",
        "thirdparty/libpng/pngwrite.c",
        "thirdparty/libpng/pngwtran.c",
        "thirdparty/libpng/pngwutil.c",
    }
    if use_brotli then
        includedirs { "thirdparty/brotli/include" }
        if opt_bool("asan") or opt_bool("ubsan") or opt_bool("tsan") or opt_bool("lsan") then
            defines { "BROTLI_BUILD_PORTABLE" }
        end
        files {
            "thirdparty/brotli/common/constants.c",
            "thirdparty/brotli/common/context.c",
            "thirdparty/brotli/common/dictionary.c",
            "thirdparty/brotli/common/platform.c",
            "thirdparty/brotli/common/shared_dictionary.c",
            "thirdparty/brotli/common/transform.c",
            "thirdparty/brotli/dec/bit_reader.c",
            "thirdparty/brotli/dec/decode.c",
            "thirdparty/brotli/dec/huffman.c",
            "thirdparty/brotli/dec/prefix.c",
            "thirdparty/brotli/dec/state.c",
            "thirdparty/brotli/dec/static_init.c",
        }
    end
    if target_platform == "android" then
        files { "thirdparty/misc/ifaddrs-android.cc" }
    end
    -- Architecture-specific libpng SIMD files (drivers/png/SCsub)
    filter "platforms:x86_64 or x86_32"
        defines { "PNG_INTEL_SSE" }
        files {
            "thirdparty/libpng/intel/filter_sse2_intrinsics.c",
            "thirdparty/libpng/intel/intel_init.c",
        }
    filter "platforms:arm64 or arm32"
        if is_msvc then
            defines { "PNG_ARM_NEON_OPT=0" }
        else
            files {
                "thirdparty/libpng/arm/arm_init.c",
                "thirdparty/libpng/arm/filter_neon_intrinsics.c",
                "thirdparty/libpng/arm/palette_neon_intrinsics.c",
            }
        end
    filter "platforms:loongarch64"
        files {
            "thirdparty/libpng/loongarch/filter_lsx_intrinsics.c",
            "thirdparty/libpng/loongarch/loongarch_lsx_init.c",
        }
    filter "platforms:ppc64"
        files {
            "thirdparty/libpng/powerpc/filter_vsx_intrinsics.c",
            "thirdparty/libpng/powerpc/powerpc_init.c",
        }
    filter {}
    -- GLAD OpenGL/EGL/GLX loader (desktop platforms when opengl3 is enabled)
    if use_opengl3 and (target_platform == "linuxbsd" or target_platform == "windows" or target_platform == "macos") then
        includedirs { "thirdparty/glad" }
        defines { "GLAD_ENABLED" }
        files { "thirdparty/glad/gl.c" }
        if target_platform == "linuxbsd" then
            defines { "EGL_ENABLED", "GLAD_GLES2" }
            files { "thirdparty/glad/egl.c" }
            if opt_bool("x11") then
                defines { "GLAD_GLX_NO_X11" }
                files { "thirdparty/glad/glx.c" }
            end
        elseif use_angle then
            defines { "EGL_ENABLED", "GLAD_GLES2" }
        end
    end
    -- Vulkan volk & re-spirv
    if use_vulkan then
        includedirs { "thirdparty/vulkan", "thirdparty/vulkan/include" }
        apply_vulkan_platform_defines()
        files { "thirdparty/re-spirv/re-spirv.cpp" }
        if use_volk then
            includedirs { "thirdparty/volk" }
            defines { "USE_VOLK" }
            files { "thirdparty/volk/volk.c" }
        end
    end
register_tp("tp_core")

-- =============================================================================
-- 6. MAIN ENGINE TARGET ("voltaire")
-- =============================================================================

project "voltaire"
    if target_platform == "android" or library_type == "shared_library" then
        kind "SharedLib"
    elseif target_platform == "ios" or target_platform == "visionos" or library_type == "static_library" then
        kind "StaticLib"
    elseif target_platform == "windows" and _OPTIONS["windows-subsystem"] == "gui" then
        kind "WindowedApp"
    else
        kind "ConsoleApp"
    end

    if target_platform == "android" then
        targetname "godot_android"
    elseif target_platform == "ios" or target_platform == "visionos" then
        targetname "voltaire_engine"
    elseif target_platform == "web" then
        targetextension ".js"
    end

    apply_common_settings()
    apply_engine_warnings()

    -- SCons-free codegen prebuild check (uses scripts/generate_headers.py if generated headers are absent;
    -- subshell + zero single quotes guarantees 100% Premake Ninja & Make compatibility!)
    if host_os ~= "windows" then
        prebuildcommands {
            [[(if [ -f ../premake5.lua ]; then cd ..; fi; test -f core/version_generated.gen.h || python3 scripts/generate_headers.py --if-needed)]],
        }
    end

    includedirs {
        "drivers/sdl",
        "thirdparty",
        "thirdparty/amd-fsr2",
        "thirdparty/astcenc",
        "thirdparty/basis_universal",
        "thirdparty/brotli/include",
        "thirdparty/clipper2/include",
        "thirdparty/cvtt",
        "thirdparty/embree",
        "thirdparty/embree/include",
        "thirdparty/enet",
        "thirdparty/etcpak",
        "thirdparty/freetype/include",
        "thirdparty/glad",
        "thirdparty/glslang",
        "thirdparty/graphite/include",
        "thirdparty/harfbuzz/src",
        "thirdparty/icu4c",
        "thirdparty/icu4c/common",
        "thirdparty/icu4c/i18n",
        "thirdparty/jolt_physics",
        "thirdparty/libjpeg-turbo/src",
        "thirdparty/libktx/external",
        "thirdparty/libktx/include",
        "thirdparty/libktx/lib",
        "thirdparty/libktx/other_include",
        "thirdparty/libktx/utils",
        "thirdparty/libogg",
        "thirdparty/libpng",
        "thirdparty/libtheora",
        "thirdparty/libvorbis",
        "thirdparty/libwebp",
        "thirdparty/libwebp/src",
        "thirdparty/manifold/include",
        "thirdparty/mbedtls/godot",
        "thirdparty/mbedtls/include",
        "thirdparty/mbedtls/tf-psa-crypto/drivers/builtin/include",
        "thirdparty/mbedtls/tf-psa-crypto/include",
        "thirdparty/miniupnpc/include",
        "thirdparty/msdfgen",
        "thirdparty/openxr",
        "thirdparty/openxr/include",
        "thirdparty/openxr/src",
        "thirdparty/openxr/src/common",
        "thirdparty/openxr/src/external/jsoncpp/include",
        "thirdparty/pcre2/src",
        "thirdparty/recastnavigation/Recast/Include",
        "thirdparty/sdl",
        "thirdparty/sdl/include",
        "thirdparty/sdl/include/build_config",
        "thirdparty/spirv-headers/include",
        "thirdparty/spirv-headers/include/spirv/unified1",
        "thirdparty/thorvg/inc",
        "thirdparty/thorvg/src/common",
        "thirdparty/thorvg/src/renderer",
        "thirdparty/tinyexr",
        "thirdparty/ufbx",
        "thirdparty/vhacd/inc",
        "thirdparty/volk",
        "thirdparty/vulkan",
        "thirdparty/vulkan/include",
        "thirdparty/wslay",
        "thirdparty/xatlas",
        "thirdparty/zlib",
        "thirdparty/zstd",
    }
    if target_platform == "linuxbsd" then
        includedirs { "thirdparty/sdl/core/linux" }
    end

    defines(mbedtls_defs)
    defines {
        "GODOT_MODULE",
        "VMA_EXTERNAL_MEMORY_WIN32=0",
        "SDL_PLATFORM_PRIVATE",
        "MANIFOLD_PAR=-1",
        "GODOT_ENET",
        "ENABLE_OPT=0",
        "KHRONOS_STATIC=1",
        "MSDFGEN_PUBLIC=",
        "EMBREE_TARGET_SSE2",
        "EMBREE_LOWEST_ISA",
        "TASKING_INTERNAL",
        "PCRE2_STATIC",
        "PCRE2_CODE_UNIT_WIDTH=0",
        "TVG_STATIC",
        "THORVG_FILE_IO_SUPPORT",
        "THORVG_WEBP_LOADER_SUPPORT",
        "THORVG_JPG_LOADER_SUPPORT",
        "U_STATIC_IMPLEMENTATION",
        "U_HAVE_LIB_SUFFIX=1",
        "U_LIB_SUFFIX_C_NAME=_godot",
        "HAVE_ICU_BUILTIN",
        "TINYEXR_USE_THREAD",
        "TINYEXR_USE_MINIZ=0",
    }
    if not is_msvc then
        defines { "FFX_GCC" }
    end
    if use_brotli then
        defines { "FT_CONFIG_OPTION_USE_BROTLI" }
    end
    if not opt_bool("mp3-extra-formats") then
        defines { "DR_MP3_ONLY_MP3" }
    end
    if use_vulkan then
        apply_vulkan_platform_defines()
        if use_volk then defines { "USE_VOLK" } end
    else
        defines { "LIBKTX" }
    end
    if use_opengl3 and (target_platform == "linuxbsd" or target_platform == "windows" or target_platform == "macos") then
        defines { "GLAD_ENABLED" }
        if target_platform == "linuxbsd" then
            defines { "EGL_ENABLED", "GLAD_GLES2" }
            if opt_bool("x11") then defines { "GLAD_GLX_NO_X11" } end
        elseif use_angle then
            defines { "EGL_ENABLED", "GLAD_GLES2" }
        end
    end
    if target_platform ~= "web" then
        defines { "MINIUPNP_STATICLIB", "HAVE_CONFIG_H" }
        if target_platform == "windows" then
            defines { "HAVE_WINSOCK2_H" }
        else
            defines { "MINIUPNPC_SET_SOCKET_TIMEOUT", "HAVE_NETINET_IN_H" }
        end
    end
    if _OPTIONS["precision"] == "double" then
        defines { "JPH_DOUBLE_PRECISION" }
    end
    if is_dev_build then
        defines { "JPH_ENABLE_ASSERTS" }
    end

    filter "configurations:editor"
        defines {
            "JPH_DEBUG_RENDERER",
            "MINIZ_HEADER_FILE_ONLY",
            "ICU_STATIC_DATA",
        }
    filter {}

    -- -------------------------------------------------------------------------
    -- Engine Source Files (core, main, scene, servers, common drivers & modules)
    -- -------------------------------------------------------------------------
    files {
        -- Core & MbedTLS Godot platform glue
        "core/crypto/crypto.cpp",
        "core/input/default_controller_mappings.gen.cpp",
        "core/input/input.cpp",
        "core/io/file_access.cpp",
        "core/io/image.cpp",
        "core/io/resource_loader.cpp",
        "core/io/stream_peer.cpp",
        "core/math/convex_hull.cpp",
        "core/math/geometry_3d.cpp",
        "core/math/math_primitives.cpp",
        "core/math/math_transforms.cpp",
        "core/math/math_vectors.cpp",
        "core/os/os.cpp",
        "core/register_core_types.cpp",
        "core/script_encryption_key.gen.cpp",
        "core/string/translation.cpp",
        "core/string/ustring.cpp",
        "core/version_hash.gen.cpp",
        "thirdparty/mbedtls/godot/godot_mbedtls_platform.cpp",

        -- Main
        "main/main.cpp",

        -- Scene (always-enabled)
        "scene/2d/node_2d.cpp",
        "scene/2d/physics/physics_body_2d.cpp",
        "scene/animation/animation_tree.cpp",
        "scene/gui/control.cpp",
        "scene/gui/dialogs.cpp",
        "scene/gui/text_edit.cpp",
        "scene/main/scene_tree.cpp",
        "scene/register_scene_types.cpp",
        "scene/resources/material.cpp",
        "scene/resources/mesh.cpp",
        "scene/resources/texture.cpp",

        -- Servers (always-enabled)
        "servers/physics_2d/physics_server_2d.cpp",
        "servers/physics_3d/physics_server_3d.cpp",
        "servers/register_server_types.cpp",
        "servers/rendering/renderer.cpp",
        "servers/rendering/renderer_rd/effects/copy_effects.cpp",
        "servers/rendering/renderer_rd/forward_clustered/render_forward_clustered.cpp",
        "servers/rendering/renderer_rd/forward_mobile/render_forward_mobile.cpp",
        "servers/rendering/renderer_rd/renderer_compositor_rd.cpp",
        "servers/rendering/renderer_rd/renderer_scene_render_rd.cpp",
        "servers/rendering/renderer_rd/storage_rd/texture_storage.cpp",
        "servers/rendering/rendering_device.cpp",
        "servers/rendering/shader_language.cpp",

        -- Drivers (common)
        "drivers/alsa/asound-so_wrap.c",
        "drivers/png/image_loader_png.cpp",
        "drivers/register_driver_types.cpp",
        "drivers/unix/os_unix.cpp",

        -- Platform API registry (common across all platforms)
        "platform/android/api/api.cpp",
        "platform/register_platform_apis.gen.cpp",

        -- Modules (always-enabled)
        "modules/astcenc/image_compress_astcenc.cpp",
        "modules/basis_universal/image_compress_basisu.cpp",
        "modules/bcdec/image_decompress_bcdec.cpp",
        "modules/bmp/image_loader_bmp.cpp",
        "modules/dds/texture_loader_dds.cpp",
        "modules/enet/enet_connection.cpp",
        "modules/etcpak/image_compress_etcpak.cpp",
        "modules/hdr/image_loader_hdr.cpp",
        "modules/interactive_music/audio_stream_interactive.cpp",
        "modules/jpg/image_loader_libjpeg_turbo.cpp",
        "modules/ktx/texture_loader_ktx.cpp",
        "modules/mbedtls/crypto_mbedtls.cpp",
        "modules/mbedtls/mbedtls_cpp_compat.c",
        "modules/mp3/audio_stream_mp3.cpp",
        "modules/multiplayer/scene_multiplayer.cpp",
        "modules/noise/fastnoise_lite.cpp",
        "modules/ogg/ogg_packet_sequence.cpp",
        "modules/regex/regex.cpp",
        "modules/register_module_types.gen.cpp",
        "modules/svg/image_loader_svg.cpp",
        "modules/text_server_adv/text_server_adv.cpp",
        "modules/tga/image_loader_tga.cpp",
        "modules/tinyexr/image_loader_tinyexr.cpp",
        "modules/upnp/upnp.cpp",
        "modules/vorbis/audio_stream_ogg_vorbis.cpp",
        "modules/webp/image_loader_webp.cpp",
        "modules/webrtc/webrtc_peer_connection.cpp",
        "modules/websocket/websocket_peer.cpp",
        "modules/zip/zip_reader.cpp",
    }

    -- Feature-conditional scene/server/module files
    if not disable_3d then
        files {
            "scene/3d/node_3d.cpp",
            "scene/3d/physics/physics_body_3d.cpp",
            "scene/3d/visual_instance_3d.cpp",
            "scene/resources/3d/primitive_meshes.cpp",
            "modules/csg/csg_shape.cpp",
            "modules/fbx/fbx_document.cpp",
            "modules/gltf/gltf_document.cpp",
            "modules/gridmap/grid_map.cpp",
            "modules/meshoptimizer/register_types.cpp",
        }
    end
    if not disable_phys_2d then
        files { "modules/godot_physics_2d/godot_physics_server_2d.cpp" }
    end
    if not disable_phys_3d then
        files {
            "modules/jolt_physics/jolt_physics_server_3d.cpp",
            "modules/vhacd/register_types.cpp",
        }
    end
    if not disable_nav_2d then
        files {
            "scene/2d/navigation/navigation_2d.cpp",
            "servers/navigation_2d/navigation_server_2d.cpp",
            "modules/navigation_2d/nav_map_2d.cpp",
        }
    end
    if not disable_nav_3d then
        files {
            "scene/3d/navigation/navigation_3d.cpp",
            "servers/navigation_3d/navigation_server_3d.cpp",
            "modules/navigation_3d/nav_map_3d.cpp",
        }
    end
    if not disable_xr then
        files {
            "scene/3d/xr/xr_nodes.cpp",
            "servers/xr/xr_server.cpp",
            "modules/mobile_vr/mobile_vr_interface.cpp",
        }
    end
    if has_webxr then
        files { "modules/webxr/webxr_interface.cpp" }
    end
    if use_opengl3 then
        files {
            "drivers/gles3/rasterizer_canvas_gles3.cpp",
            "drivers/gles3/rasterizer_scene_gles3.cpp",
            "drivers/gles3/storage/utilities.cpp",
        }
    end
    if use_vulkan then
        files { "drivers/vulkan/rendering_device_driver_vulkan.cpp" }
    end
    if use_rd then
        files { "modules/glslang/register_types.cpp" }
    end
    if use_sdl then
        files { "drivers/sdl/joypad_sdl.cpp" }
    end
    if has_theora then
        filter "platforms:not rv64"
            files { "modules/theora/video_stream_theora.cpp" }
        filter {}
    end
    for _, a in ipairs(active_arches) do
        if arch_supports_raycast(a) then
            filter("platforms:" .. a)
                files { "modules/raycast/raycast_occlusion_cull.cpp" }
            filter {}
        end
    end
    if has_camera then
        files { "modules/camera/register_types.cpp" }
        if target_platform == "linuxbsd" then
            files { "modules/camera/camera_linux.cpp" }
        elseif target_platform == "windows" then
            files { "modules/camera/camera_win.cpp" }
        elseif target_platform == "macos" or target_platform == "ios" or target_platform == "visionos" then
            files { "modules/camera/camera_apple.mm" }
        elseif target_platform == "android" then
            files { "modules/camera/camera_android.cpp" }
        end
    end
    if has_openxr then
        files { "modules/openxr/openxr_api.cpp" }
        if target_platform == "android" then
            files { "modules/openxr/extensions/platform/openxr_android_extension.cpp" }
        end
        if use_vulkan then
            files { "modules/openxr/extensions/platform/openxr_vulkan_extension.cpp" }
        end
        if use_opengl3 and target_platform ~= "macos" then
            files { "modules/openxr/extensions/platform/openxr_opengl_extension.cpp" }
        end
        if use_d3d12 then
            files { "modules/openxr/extensions/platform/openxr_d3d12_extension.cpp" }
        end
        if use_metal then
            filter "platforms:arm64"
                files { "modules/openxr/extensions/platform/openxr_metal_extension.mm" }
            filter {}
        end
    end
    if betsy_templates then
        files { "modules/betsy/image_compress_betsy.cpp" }
    else
        filter "configurations:editor"
            files { "modules/betsy/image_compress_betsy.cpp" }
        filter {}
    end
    if cvtt_templates then
        files { "modules/cvtt/image_compress_cvtt.cpp" }
    else
        filter "configurations:editor"
            files { "modules/cvtt/image_compress_cvtt.cpp" }
        filter {}
    end

    -- Editor-only sources & platform exporters
    filter "configurations:editor"
        files {
            "editor/editor_ui.cpp",
            "editor/export/editor_export.cpp",
            "editor/export/register_exporters.gen.cpp",
            "editor/import/3d/resource_importer_scene.cpp",
            "editor/project_upgrade/renames_map_3_to_4.cpp",
            "editor/translations/doc_translations.gen.cpp",
            "editor/translations/editor_translations.gen.cpp",
            "editor/translations/extractable_translations.gen.cpp",
            "editor/translations/property_translations.gen.cpp",
            "modules/lightmapper_rd/lightmapper_rd.cpp",
            "modules/xatlas_unwrap/register_types.cpp",
            "platform/android/export/export.cpp",
            "platform/ios/export/export.cpp",
            "platform/linuxbsd/export/export.cpp",
            "platform/macos/export/export.cpp",
            "platform/visionos/export/export.cpp",
            "platform/web/export/export.cpp",
            "platform/windows/export/export.cpp",
        }
    filter {}

    -- -------------------------------------------------------------------------
    -- Platform-Specific Engine Sources, Link Flags & System Libraries
    -- -------------------------------------------------------------------------

    -- Link built-in StaticLib targets first (in topological order)
    links(linked_tp_libs)
    if not cvtt_templates and #cvtt_tp_libs > 0 then
        filter "configurations:editor"
            links(cvtt_tp_libs)
        filter {}
    end
    if #editor_only_tp_libs > 0 then
        filter "configurations:editor"
            links(editor_only_tp_libs)
        filter {}
    end

    if target_platform == "linuxbsd" then
        if host_os == "linux" then
            linkgroups "On"
        end
        defines {
            "SDL_PLATFORM_LINUX",
            "XR_OS_LINUX",
        }
        if opt_bool("x11") and has_openxr then defines { "XR_USE_PLATFORM_XLIB" } end
        if opt_bool("wayland") and use_opengl3 and has_openxr then
            defines { "XR_USE_PLATFORM_EGL", "XRDEPENDENCIES_USE_GLAD" }
        end
        if host_os ~= "bsd" then defines { "HAVE_SECURE_GETENV" } end
        filter "platforms:x86_64 or x86_32"
            defines { "OC_X86_ASM" }
        filter {}

        files { "platform/linuxbsd/os_linuxbsd.cpp" }
        if library_type == "executable" then
            files { "platform/linuxbsd/godot_linuxbsd.cpp" }
        else
            files { "platform/linuxbsd/libgodot_linuxbsd.cpp" }
        end
        if opt_bool("sowrap") then
            files { "platform/linuxbsd/xkbcommon-so_wrap.c" }
        end
        if opt_bool("x11") then
            files { "platform/linuxbsd/x11/display_server_x11.cpp" }
            if opt_bool("sowrap") then
                files { "platform/linuxbsd/x11/dynwrappers/xlib-so_wrap.c" }
            end
            if use_opengl3 then
                files { "platform/linuxbsd/x11/gl_manager_x11.cpp" }
            end
        end
        if opt_bool("wayland") then
            files {
                "platform/linuxbsd/wayland/display_server_wayland.cpp",
                "platform/linuxbsd/wayland/protocol/wayland_protocols.gen.c",
            }
            if opt_bool("sowrap") then
                files { "platform/linuxbsd/wayland/dynwrappers/wayland-client-core-so_wrap.c" }
            end
            if use_opengl3 then
                files { "platform/linuxbsd/wayland/egl_manager_wayland.cpp" }
            end
        end

        if use_accesskit then
            filter "platforms:x86_64"
                libdirs { accesskit_sdk_path .. "/lib/linux/x86_64/static" }
            filter "platforms:x86_32"
                libdirs { accesskit_sdk_path .. "/lib/linux/x86/static" }
            filter "platforms:arm64"
                libdirs { accesskit_sdk_path .. "/lib/linux/arm64/static" }
            filter "platforms:arm32"
                libdirs { accesskit_sdk_path .. "/lib/linux/arm32/static" }
            filter "platforms:rv64"
                libdirs { accesskit_sdk_path .. "/lib/linux/riscv64gc/static" }
            filter {}
            links { "accesskit" }
        end

        if use_static_cpp then
            linkoptions { "-static-libgcc", "-static-libstdc++" }
        end
        if _OPTIONS["linker"] ~= "default" then
            linkoptions { "-fuse-ld=" .. _OPTIONS["linker"] }
        end
        if lto_mode == "thin" then
            linkoptions { "-flto=thin" }
        elseif lto_mode == "full" then
            linkoptions { "-flto" }
        end
        if opt_bool("asan") then linkoptions { "-fsanitize=address" } end
        if opt_bool("ubsan") then linkoptions { "-fsanitize=undefined" } end
        if opt_bool("tsan") then linkoptions { "-fsanitize=thread" } end
        if opt_bool("lsan") then linkoptions { "-fsanitize=leak" } end

        if host_os ~= "bsd" then
            links { "atomic" }
        end
        if opt_bool("wayland") then
            links { "rt" }
        end
        links { "pthread" }
        if host_os == "linux" then
            links { "dl" }
        elseif host_os == "bsd" then
            linkoptions { "-lkvm" }
        end
        if opt_bool("execinfo") then
            links { "execinfo" }
        end
        links { "m" }
        if not opt_bool("sowrap") then
            if opt_bool("x11") then
                links { "X11", "Xcursor", "Xinerama", "Xext", "Xrandr", "Xrender", "Xi" }
            end
            if opt_bool("wayland") then
                links { "wayland-client", "wayland-cursor", "wayland-egl" }
                if opt_bool("libdecor") then links { "decor-0" } end
            end
            links { "xkbcommon" }
            if opt_bool("alsa") then links { "asound" } end
            if opt_bool("pulseaudio") then links { "pulse" } end
            if opt_bool("dbus") and use_threads then links { "dbus-1" } end
            if opt_bool("speechd") then links { "speechd" } end
            if opt_bool("fontconfig") then links { "fontconfig" } end
            if host_os == "linux" and opt_bool("udev") then links { "udev" } end
        end
        if use_vulkan and not use_volk then
            links { "vulkan" }
        end

    elseif target_platform == "windows" then
        defines {
            "SDL_PLATFORM_WINDOWS",
            "XR_OS_WINDOWS",
            "XR_USE_PLATFORM_WIN32",
        }
        filter "platforms:x86_32"
            defines { "OC_X86_ASM" }
        filter {}
        if not is_msvc then
            includedirs { "thirdparty/libbacktrace" }
        end

        files {
            "platform/windows/os_windows.cpp",
            "platform/windows/display_server_windows.cpp",
            "platform/windows/key_mapping_windows.cpp",
            "platform/windows/windows_terminal_logger.cpp",
            "platform/windows/windows_utils.cpp",
            "platform/windows/native_menu_windows.cpp",
            "platform/windows/gl_manager_windows_native.cpp",
            "platform/windows/wgl_detect_version.cpp",
            "platform/windows/rendering_context_driver_vulkan_windows.cpp",
            "platform/windows/drop_target_windows.cpp",
            "drivers/wasapi/audio_driver_wasapi.cpp",
        }
        if use_angle then
            files { "platform/windows/gl_manager_windows_angle.cpp" }
        end
        if use_xaudio2 then
            files { "drivers/xaudio2/audio_driver_xaudio2.cpp" }
        end
        if use_d3d12 then
            apply_d3d12_mesa_settings()
            files {
                "drivers/d3d12/d3d12_hooks.cpp",
                "drivers/d3d12/dxil_hash.cpp",
                "drivers/d3d12/rendering_context_driver_d3d12.cpp",
                "drivers/d3d12/rendering_device_driver_d3d12.cpp",
                "drivers/d3d12/rendering_shader_container_d3d12.cpp",
            }
        end
        if library_type == "executable" then
            files { "platform/windows/godot_windows.cpp" }
        else
            files { "platform/windows/libgodot_windows.cpp" }
        end
        if is_msvc then
            files { "platform/windows/crash_handler_windows_seh.cpp" }
        else
            files { "platform/windows/crash_handler_windows_signal.cpp" }
        end
        filter "configurations:editor"
            files { "platform/windows/godot_res.rc" }
        filter "configurations:template_debug or template_release"
            files { "platform/windows/godot_res_template.rc" }
        filter {}

        local stack_bytes = opt_bool("asan") and tostring(30 * 1024 * 1024) or "8388608"
        if is_msvc then
            linkoptions {
                "/INCREMENTAL:NO",
                "/STACK:" .. stack_bytes,
                "/NATVIS:../platform/windows/godot.natvis",
            }
            if lto_mode == "full" then linkoptions { "/LTCG" } end
            if opt_bool("asan") then linkoptions { "/INFERASANLIBS" } end
            if library_type == "executable" then
                filter "platforms:x86_64"
                    linkoptions { "/ENTRY:ShimMainCRTStartup" }
                filter { "platforms:not x86_64", "configurations:template_release" }
                    linkoptions { "/ENTRY:mainCRTStartup" }
                filter {}
            end
            links {
                "winmm", "dsound", "kernel32", "ole32", "oleaut32", "sapi",
                "user32", "gdi32", "IPHLPAPI", "Shlwapi", "Shcore", "wsock32",
                "Ws2_32", "shell32", "advapi32", "dinput8", "dxguid", "imm32",
                "bcrypt", "Crypt32", "Avrt", "dwmapi", "dwrite", "wbemuuid",
                "ntdll", "hid", "mincore",
            }
        else
            linkgroups "On"
            linkoptions { "-Wl,--stack," .. stack_bytes }
            if use_static_cpp then
                linkoptions { "-static" }
                filter "platforms:x86_32"
                    linkoptions { "-static-libgcc", "-static-libstdc++" }
                filter {}
            end
            if library_type == "executable" then
                filter "platforms:x86_64"
                    linkoptions { "-Wl,--entry=ShimMainCRTStartup" }
                filter {}
            end
            if lto_mode == "thin" then
                linkoptions { "-flto=thin" }
            elseif lto_mode == "full" then
                linkoptions { "-flto" }
                if not use_llvm then linkoptions { "-fno-use-linker-plugin", "-fwhole-program" } end
            end
            if opt_bool("asan") then linkoptions { "-fsanitize=address" } end
            if opt_bool("ubsan") then linkoptions { "-fsanitize=undefined", "-fno-sanitize=vptr" } end
            links {
                "mingw32", "dsound", "ole32", "d3d9", "winmm", "gdi32",
                "iphlpapi", "shell32", "shlwapi", "shcore", "wsock32", "ws2_32",
                "kernel32", "oleaut32", "sapi", "dinput8", "dxguid", "ksuser",
                "imm32", "bcrypt", "crypt32", "avrt", "uuid", "dwmapi", "dwrite",
                "wbemuuid", "ntdll", "hid", "mincore", "user32", "advapi32",
            }
        end
        if has_raycast_any then
            links { "psapi" }
        end
        filter "configurations:editor or template_debug"
            links { "psapi", "dbghelp" }
        filter {}
        if use_accesskit then
            for _, a in ipairs(active_arches) do
                local ak_arch = (a == "x86_32") and "x86" or a
                local ak_tc = is_msvc and "msvc" or (use_llvm and "mingw-llvm" or "mingw")
                filter("platforms:" .. a)
                    libdirs { accesskit_sdk_path .. "/lib/windows/" .. ak_arch .. "/" .. ak_tc .. "/static", "platform/windows" }
                filter {}
            end
            links { "accesskit", "uiautomationcore", "runtimeobject", "propsys", "userenv" }
        end
        if use_d3d12 then
            for _, a in ipairs(active_arches) do
                local suf = is_msvc and "-msvc" or (use_llvm and "-llvm" or "-gcc")
                local mdir = os.isdir(mesa_libs_base .. "-" .. a .. suf) and (mesa_libs_base .. "-" .. a .. suf .. "/bin") or (mesa_libs_base .. "/bin")
                filter("platforms:" .. a)
                    libdirs { mdir }
                    links { "libNIR.windows." .. a }
                filter {}
            end
            links { "dxgi", "dxguid", "version" }
        end
        if use_angle then
            for _, a in ipairs(active_arches) do
                local adir = resolve_angle_dir(a)
                if adir then
                    filter("platforms:" .. a)
                        libdirs { adir }
                        if is_msvc then
                            links { "libANGLE.windows." .. a, "libEGL.windows." .. a, "libGLES.windows." .. a }
                        else
                            links { "EGL.windows." .. a, "GLES.windows." .. a, "ANGLE.windows." .. a }
                        end
                    filter {}
                end
            end
            links { "dxgi", "d3d9", "d3d11" }
        end
        if use_vulkan and not use_volk then
            links { "vulkan" }
        end

    elseif target_platform == "macos" then
        defines { "SDL_PLATFORM_MACOS", "XR_OS_APPLE" }
        filter "platforms:x86_64"
            defines { "OC_X86_ASM" }
        filter {}

        files {
            "drivers/apple/foundation_helpers.mm",
            "drivers/apple/os_log_logger.cpp",
            "drivers/apple/thread_apple.cpp",
            "drivers/coreaudio/audio_driver_coreaudio.mm",
            "drivers/coremidi/midi_driver_coremidi.mm",
            "platform/macos/os_macos.mm",
            "platform/macos/godot_application.mm",
            "platform/macos/godot_application_delegate.mm",
            "platform/macos/crash_handler_macos.mm",
            "platform/macos/display_server_macos_base.mm",
            "platform/macos/display_server_macos.mm",
            "platform/macos/godot_button_view.mm",
            "platform/macos/godot_content_view.mm",
            "platform/macos/godot_core_cursor.mm",
            "platform/macos/godot_status_item.mm",
            "platform/macos/godot_window_delegate.mm",
            "platform/macos/godot_window.mm",
            "platform/macos/key_mapping_macos.mm",
            "platform/macos/godot_menu_delegate.mm",
            "platform/macos/godot_menu_item.mm",
            "platform/macos/godot_open_save_delegate.mm",
            "platform/macos/native_menu_macos.mm",
            "platform/macos/dir_access_macos.mm",
            "platform/macos/tts_macos.mm",
            "platform/macos/rendering_context_driver_vulkan_macos.mm",
            "platform/macos/gl_manager_macos_legacy.mm",
            "platform/macos/godot_progress_view.mm",
        }
        if use_angle then
            files { "platform/macos/gl_manager_macos_angle.mm" }
        end
        if library_type == "executable" then
            files { "platform/macos/godot_main_macos.mm" }
        else
            files { "platform/macos/libgodot_macos.mm" }
        end
        filter "configurations:editor"
            files {
                "platform/macos/display_server_macos_embedded.mm",
                "platform/macos/embedded_debugger.mm",
                "platform/macos/embedded_gl_manager.mm",
            }
        filter {}

        if apple_sdk_path ~= "" then
            linkoptions { "-isysroot", apple_sdk_path }
        end
        if os.getenv("APPLE_LLVM_CROSS") then
            filter "platforms:arm64"
                linkoptions { "-target", "arm64-apple-darwin" }
            filter "platforms:x86_64"
                linkoptions { "-target", "x86_64-apple-darwin" }
            filter {}
        end
        filter "platforms:arm64"
            linkoptions { "-arch", "arm64", "-mmacosx-version-min=13.0" }
        filter "platforms:x86_64"
            linkoptions { "-arch", "x86_64", "-mmacosx-version-min=11.0" }
        filter {}
        if library_type == "executable" then
            local stack_hex = (opt_bool("asan") or opt_bool("ubsan") or opt_bool("tsan")) and "0x1e00000" or "0x800000"
            linkoptions { "-Wl,-stack_size," .. stack_hex }
        end
        linkoptions {
            "-Wl,-rpath,@executable_path/../Frameworks",
            "-Wl,-rpath,@executable_path",
            "-framework Cocoa",
            "-framework Carbon",
            "-framework AudioUnit",
            "-framework CoreAudio",
            "-framework CoreMIDI",
            "-framework IOKit",
            "-framework GameController",
            "-framework CoreHaptics",
            "-framework CoreVideo",
            "-framework AVFoundation",
            "-framework CoreMedia",
            "-framework QuartzCore",
            "-framework Security",
            "-framework UniformTypeIdentifiers",
            "-framework IOSurface",
        }
        if use_sdl then
            linkoptions { "-framework ForceFeedback" }
        end
        if use_vulkan or use_angle or use_metal then
            linkoptions { "-framework Metal" }
        end
        if use_metal then
            filter "platforms:arm64"
                linkoptions { "-framework MetalKit", "-framework MetalFX" }
            filter {}
        end
        if use_vulkan and not use_volk then
            local vk_sdk = _OPTIONS["vulkan-sdk-path"] or os.getenv("VULKAN_SDK") or ""
            if vk_sdk ~= "" then
                libdirs { vk_sdk .. "/macOS/lib", vk_sdk .. "/MoltenVK/MoltenVK.xcframework/macos-arm64_x86_64" }
            end
            links { "MoltenVK" }
        end
        if use_accesskit then
            filter "platforms:arm64"
                libdirs { accesskit_sdk_path .. "/lib/macos/arm64/static" }
            filter "platforms:x86_64"
                libdirs { accesskit_sdk_path .. "/lib/macos/x86_64/static" }
            filter {}
            links { "accesskit" }
        end
        if use_angle then
            for _, a in ipairs(active_arches) do
                local adir = resolve_angle_dir(a)
                if adir then
                    filter("platforms:" .. a)
                        libdirs { adir }
                        links { "ANGLE.macos." .. a, "EGL.macos." .. a, "GLES.macos." .. a }
                    filter {}
                end
            end
        end
        if lto_mode == "thin" then linkoptions { "-flto=thin" }
        elseif lto_mode == "full" then linkoptions { "-flto" } end
        filter "configurations:template_release"
            if not opt_bool("debug-symbols") then
                linkoptions { "-Wl,-S", "-Wl,-dead_strip" }
            end
        filter {}
        links { "pthread", "z" }

    elseif target_platform == "android" then
        linkgroups "On"
        defines { "XR_OS_ANDROID", "XR_USE_PLATFORM_ANDROID", "JSON_USE_EXCEPTION=0" }
        files {
            "platform/android/os_android.cpp",
            "platform/android/android_input_handler.cpp",
            "platform/android/file_access_android.cpp",
            "platform/android/file_access_filesystem_jandroid.cpp",
            "platform/android/audio_driver_opensl.cpp",
            "platform/android/dir_access_jandroid.cpp",
            "platform/android/tts_android.cpp",
            "platform/android/thread_jandroid.cpp",
            "platform/android/net_socket_android.cpp",
            "platform/android/java_godot_lib_jni.cpp",
            "platform/android/java_class_wrapper.cpp",
            "platform/android/java_godot_wrapper.cpp",
            "platform/android/java_godot_view_wrapper.cpp",
            "platform/android/java_godot_io_wrapper.cpp",
            "platform/android/jni_utils.cpp",
            "platform/android/android_keys_utils.cpp",
            "platform/android/display_server_android.cpp",
            "platform/android/plugin/godot_plugin_jni.cpp",
            "platform/android/rendering_context_driver_vulkan_android.cpp",
            "platform/android/variant/callable_jni.cpp",
            "platform/android/dialog_utils_jni.cpp",
        }
        filter "platforms:arm64"
            linkoptions { "-target", "aarch64-linux-android" .. tostring(android_api) }
        filter "platforms:arm32"
            linkoptions { "-target", "armv7a-linux-androideabi" .. tostring(android_api) }
        filter "platforms:x86_64"
            linkoptions { "-target", "x86_64-linux-android" .. tostring(android_api) }
        filter "platforms:x86_32"
            linkoptions { "-target", "i686-linux-android" .. tostring(android_api) }
        filter {}
        linkoptions {
            "-Wl,--gc-sections",
            "-Wl,--no-undefined",
            "-Wl,-z,now",
            "-Wl,--build-id",
            "-Wl,-soname,libgodot_android.so",
        }
        if lto_mode == "thin" then linkoptions { "-flto=thin" }
        elseif lto_mode == "full" then linkoptions { "-flto" } end

        local swappy_map = { arm64 = "arm64-v8a", arm32 = "armeabi-v7a", x86_64 = "x86_64", x86_32 = "x86" }
        local has_swappy = os.isfile("thirdparty/swappy-frame-pacing/arm64-v8a/libswappy_static.a")
        if use_vulkan and opt_tri("swappy", has_swappy) and has_swappy then
            defines { "SWAPPY_FRAME_PACING_ENABLED" }
            for arch_k, abi_dir in pairs(swappy_map) do
                filter("platforms:" .. arch_k)
                    libdirs { "thirdparty/swappy-frame-pacing/" .. abi_dir }
                filter {}
            end
            links { "swappy_static" }
        end
        links { "OpenSLES", "EGL", "android", "log", "z", "dl", "camera2ndk", "mediandk" }
        if use_opengl3 then links { "GLESv3" } end
        if use_vulkan and not use_volk then links { "vulkan" } end

    elseif target_platform == "ios" or target_platform == "visionos" then
        defines { (target_platform == "ios") and "SDL_PLATFORM_IOS" or "SDL_PLATFORM_VISIONOS" }
        buildoptions { "-fmodules", "-fcxx-modules" }
        files {
            "drivers/apple/foundation_helpers.mm",
            "drivers/apple/os_log_logger.cpp",
            "drivers/apple/thread_apple.cpp",
            "drivers/coreaudio/audio_driver_coreaudio.mm",
            "drivers/apple_embedded/app_delegate_service.mm",
            "drivers/apple_embedded/apple_embedded.mm",
            "drivers/apple_embedded/display_server_apple_embedded.mm",
            "drivers/apple_embedded/godot_app_delegate.mm",
            "drivers/apple_embedded/godot_keyboard_input_view.mm",
            "drivers/apple_embedded/godot_view_apple_embedded.mm",
            "drivers/apple_embedded/godot_view_controller.mm",
            "drivers/apple_embedded/godot_view_renderer.mm",
            "drivers/apple_embedded/key_mapping_apple_embedded.mm",
            "drivers/apple_embedded/main_utilities.mm",
            "drivers/apple_embedded/os_apple_embedded.mm",
            "drivers/apple_embedded/rendering_context_driver_vulkan_apple_embedded.mm",
            "drivers/apple_embedded/tts_apple_embedded.mm",
        }
        if target_platform == "ios" then
            files {
                "platform/ios/device_metrics.mm",
                "platform/ios/display_layer_ios.mm",
                "platform/ios/display_server_ios.mm",
                "platform/ios/godot_view_ios.mm",
                "platform/ios/main_ios.mm",
                "platform/ios/os_ios.mm",
            }
        else
            files {
                "platform/visionos/display_layer_visionos.mm",
                "platform/visionos/display_server_visionos.mm",
                "platform/visionos/godot_view_visionos.mm",
                "platform/visionos/main_visionos.mm",
                "platform/visionos/os_visionos.mm",
            }
        end
        -- Merge all static archives into a single fat libgodot.a (matches combine_libs_apple_embedded in platform_methods.py)
        if host_os == "macosx" or os.getenv("OSXCROSS_IOS") or os.getenv("OSXCROSS_VISIONOS") then
            postbuildcommands {
                [[(cd "%{cfg.buildtarget.directory}" && libtool -static -o libgodot.a lib*.a)]],
            }
        end

    elseif target_platform == "web" then
        files {
            "platform/web/audio_driver_web.cpp",
            "platform/web/webmidi_driver.cpp",
            "platform/web/display_server_web.cpp",
            "platform/web/http_client_web.cpp",
            "platform/web/javascript_bridge_singleton.cpp",
            "platform/web/web_main.cpp",
            "platform/web/web_runtime.cpp",
            "platform/web/os_web.cpp",
        }

        local js_libs = {
            "../platform/web/js/libs/library_godot_audio.js",
            "../platform/web/js/libs/library_godot_display.js",
            "../platform/web/js/libs/library_godot_emscripten.js",
            "../platform/web/js/libs/library_godot_fetch.js",
            "../platform/web/js/libs/library_godot_webmidi.js",
            "../platform/web/js/libs/library_godot_os.js",
            "../platform/web/js/libs/library_godot_runtime.js",
            "../platform/web/js/libs/library_godot_input.js",
            "../platform/web/js/libs/library_godot_webgl2.js",
            "../modules/webrtc/library_godot_webrtc.js",
            "../modules/websocket/library_godot_websocket.js",
        }
        if opt_bool("javascript-eval") then
            table.insert(js_libs, "../platform/web/js/libs/library_godot_javascript_singleton.js")
        end
        if has_webxr then
            table.insert(js_libs, "../modules/webxr/native/library_godot_webxr.js")
        end
        for _, jlib in ipairs(js_libs) do
            linkoptions { "--js-library", jlib }
        end
        linkoptions {
            "--post-js", "../platform/web/js/patches/patch_em_gl.js",
            "-lidbfs.js",
            "-sWASM_BIGINT",
            "-sENVIRONMENT=web,worker",
            "-sMODULARIZE=1",
            "-sEXPORT_NAME=Godot",
            "-sSUPPORT_LONGJMP=wasm",
            "-sALLOW_MEMORY_GROWTH=1",
            "-sINVOKE_RUN=0",
            "-sEXIT_RUNTIME=1",
            "-sGL_WORKAROUND_SAFARI_GETCONTEXT_BUG=0",
            "-sSTACK_SIZE=" .. (_OPTIONS["stack-size"] or "5120") .. "KB",
            "-sEXPORTED_RUNTIME_METHODS=callMain,cwrap,HEAP8,HEAPU8,HEAP16,HEAPU16,HEAP32,HEAPU32,HEAP64,HEAPU64,HEAPF32,HEAPF64",
        }
        if use_opengl3 then
            linkoptions {
                "-sMAX_WEBGL_VERSION=2",
                "-sOFFSCREEN_FRAMEBUFFER=1",
                "-sGL_ENABLE_GET_PROC_ADDRESS=0",
            }
        end
        if use_threads then
            linkoptions {
                "-sUSE_PTHREADS=1",
                "-sDEFAULT_PTHREAD_STACK_SIZE=2048KB",
                "-sWASM_MEM_MAX=2048MB",
                "-sEXPORTED_FUNCTIONS=_main,_malloc,_free,__emscripten_thread_crashed",
            }
        else
            linkoptions { "-sEXPORTED_FUNCTIONS=_main,_malloc,_free" }
        end
        if opt_bool("proxy-to-pthread") and use_threads and not opt_bool("dlink") then
            linkoptions { "-sPROXY_TO_PTHREAD=1" }
        end
        if opt_bool("closure-compiler") then
            linkoptions { "--closure", "1" }
        end
        if lto_mode == "thin" then
            linkoptions { "-flto=thin", "-Wl,-u,_emscripten_run_callback_on_thread" }
        elseif lto_mode == "full" then
            linkoptions { "-flto", "-Wl,-u,_emscripten_run_callback_on_thread" }
        end
        filter "platforms:wasm32"
            linkoptions { "-sMEMORY64=0" }
        filter "platforms:wasm64"
            linkoptions { "-sMEMORY64=1" }
        filter {}
        local init_mem = tonumber(_OPTIONS["initial-memory"] or "32") or 32
        local editor_mem = (init_mem < 64) and 64 or init_mem
        filter "configurations:editor"
            linkoptions { "-sINITIAL_MEMORY=" .. tostring(editor_mem) .. "MB", "-sASSERTIONS=1" }
        filter "configurations:template_debug"
            linkoptions { "-sINITIAL_MEMORY=" .. tostring(init_mem) .. "MB", "-sASSERTIONS=1" }
        filter "configurations:template_release"
            linkoptions { "-sINITIAL_MEMORY=" .. tostring(init_mem) .. "MB" }
        filter {}
    end

-- =============================================================================
-- 7. WINDOWS CONSOLE WRAPPER COMPANION ("voltaire.console.exe")
-- =============================================================================

if target_platform == "windows" and _OPTIONS["windows-subsystem"] == "gui" and library_type == "executable" then
    project "voltaire_console"
        kind "ConsoleApp"
        targetname "voltaire.console"
        apply_common_settings()
        apply_engine_warnings()
        dependson { "voltaire" }
        files { "platform/windows/console_wrapper_windows.cpp" }
        filter "configurations:editor"
            files { "platform/windows/godot_res_wrap.rc" }
        filter "configurations:template_debug or template_release"
            files { "platform/windows/godot_res_wrap_template.rc" }
        filter {}
        links { "version" }
        if not is_msvc and use_static_cpp then
            linkoptions { "-static" }
        end
end
