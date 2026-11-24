-- =============================================================================
-- Fallback Premake5 Ninja Action Module for Voltaire Engine
--
-- Registers the `ninja` action when running on Premake5 builds that do not yet
-- bundle native Ninja support (e.g. Premake 5.0.0-dev builds prior to native
-- ninja integration). When native `ninja` is already present, this module is a
-- no-op so Premake's built-in generator is used.
-- =============================================================================

local p = premake

if p.action.get("ninja") then
    return
end

local ninja = {}

local function ninja_esc_path(str)
    if not str then return "" end
    str = str:gsub("%$", "$$")
    str = str:gsub(":", "$:")
    str = str:gsub(" ", "$ ")
    return str
end

local function ninja_esc_val(str)
    if not str then return "" end
    return (str:gsub("%$", "$$"))
end

local function sanitize_id(str)
    return (str:gsub("[^%w_]", "_"))
end

local function quote_arg(arg)
    if arg:find("[%s\"'`\\]") then
        return '"' .. arg:gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
    end
    return arg
end

local function format_define_posix(def)
    local escaped = def:gsub('\\', '\\\\'):gsub('"', '\\"')
    return '-D"' .. escaped .. '"'
end

local function format_define_msvc(def)
    local escaped = def:gsub('"', '\\"')
    if escaped:find("[%s\"<>|&]") then
        return '/D"' .. escaped .. '"'
    end
    return "/D" .. escaped
end

local function resolve_toolset(cfg)
    local name = cfg.toolset or _OPTIONS.cc or "gcc"
    local base = name:match("^([^-]+)") or name
    local ts = p.tools[name] or p.tools[base] or p.tools.gcc
    local is_msc = (ts == p.tools.msc) or (base == "msc")
    return ts, base, is_msc
end

local function get_tool_cmd(ts, cfg, kind, fallback)
    if ts and ts.gettoolname then
        local cmd = ts.gettoolname(cfg, kind)
        if cmd and cmd ~= "" then
            return cmd
        end
    end
    if ts and ts.tools and ts.tools[kind] and ts.tools[kind] ~= "" then
        local prefix = cfg.gccprefix or ""
        return prefix .. ts.tools[kind]
    end
    return fallback
end

local function is_c_like(ext)
    return ext == ".c" or ext == ".m" or ext == ".s" or ext == ".S"
end

local function is_cxx_like(ext)
    return ext == ".cpp" or ext == ".cc" or ext == ".cxx" or ext == ".mm"
end

local function is_rc(ext)
    return ext == ".rc"
end

local function normalize_rel(prj, abspath)
    if not abspath or abspath == "" then return "" end
    local rel = p.project.getrelative(prj, abspath)
    return (rel:gsub("\\", "/"))
end

local function join_flags(parts)
    local out = {}
    for _, v in ipairs(parts) do
        if v and v ~= "" then
            table.insert(out, v)
        end
    end
    return table.concat(out, " ")
end

local function emit_rules(prj)
    local emitted = {}
    for cfg in p.project.eachconfig(prj) do
        local ts, base, is_msc = resolve_toolset(cfg)
        local cc_cmd = is_msc and "cl" or get_tool_cmd(ts, cfg, "cc", base == "clang" and "clang" or "gcc")
        local cxx_cmd = is_msc and "cl" or get_tool_cmd(ts, cfg, "cxx", base == "clang" and "clang++" or "g++")
        local ar_cmd = is_msc and "lib" or get_tool_cmd(ts, cfg, "ar", "ar")
        local rc_cmd = is_msc and "rc" or get_tool_cmd(ts, cfg, "rc", "windres")
        local tag = sanitize_id(base .. (cfg.gccprefix and cfg.gccprefix ~= "" and ("_" .. cfg.gccprefix) or ""))

        if not emitted[tag] then
            emitted[tag] = true
            if is_msc then
                p.w("rule cc_" .. tag)
                p.w("  command = " .. ninja_esc_val(cc_cmd) .. " /nologo /showIncludes $cflags /c $in /Fo$out")
                p.w("  deps = msvc")
                p.w("  description = Compiling C source $in")
                p.w("")
                p.w("rule cxx_" .. tag)
                p.w("  command = " .. ninja_esc_val(cxx_cmd) .. " /nologo /showIncludes $cxxflags /c $in /Fo$out")
                p.w("  deps = msvc")
                p.w("  description = Compiling C++ source $in")
                p.w("")
                p.w("rule rc_" .. tag)
                p.w("  command = " .. ninja_esc_val(rc_cmd) .. " /nologo $resflags /fo$out $in")
                p.w("  description = Compiling resource $in")
                p.w("")
                p.w("rule ar_" .. tag)
                p.w("  command = " .. ninja_esc_val(ar_cmd) .. " /nologo /OUT:$out $in")
                p.w("  description = Archiving static library $out")
                p.w("")
                p.w("rule link_" .. tag)
                p.w("  command = link /nologo /OUT:$out $in $links $ldflags")
                p.w("  description = Linking target $out")
                p.w("")
            else
                p.w("rule cc_" .. tag)
                p.w("  command = " .. ninja_esc_val(cc_cmd) .. " $cflags -c $in -o $out -MD -MF $out.d")
                p.w("  deps = gcc")
                p.w("  depfile = $out.d")
                p.w("  description = Compiling C source $in")
                p.w("")
                p.w("rule cxx_" .. tag)
                p.w("  command = " .. ninja_esc_val(cxx_cmd) .. " $cxxflags -c $in -o $out -MD -MF $out.d")
                p.w("  deps = gcc")
                p.w("  depfile = $out.d")
                p.w("  description = Compiling C++ source $in")
                p.w("")
                p.w("rule rc_" .. tag)
                p.w("  command = " .. ninja_esc_val(rc_cmd) .. " -i $in -o $out $resflags")
                p.w("  description = Compiling resource $in")
                p.w("")
                p.w("rule ar_" .. tag)
                p.w("  command = " .. ninja_esc_val(ar_cmd) .. " -rcs $out $in")
                p.w("  description = Archiving static library $out")
                p.w("")
                p.w("rule link_" .. tag)
                p.w("  command = " .. ninja_esc_val(cxx_cmd) .. " -o $out $in $links $ldflags")
                p.w("  description = Linking target $out")
                p.w("")
            end
        end
    end

    p.w("rule copy")
    if os.ishost("windows") then
        p.w("  command = cmd /c copy /Y $in $out >nul")
    else
        p.w("  command = cp -f $in $out")
    end
    p.w("  description = Copying file $in to $out")
    p.w("")
    p.w("rule prebuild")
    p.w("  command = $prebuildcommands")
    p.w("  description = Running pre-build commands")
    p.w("")
    p.w("rule prelink")
    p.w("  command = $prelinkcommands")
    p.w("  description = Running pre-link commands")
    p.w("")
    p.w("rule postbuild")
    p.w("  command = $postbuildcommands")
    p.w("  description = Running post-build commands")
    p.w("")
    p.w("rule custom")
    p.w("  command = $customcommand")
    p.w("  description = Running custom command: $customcommand")
    p.w("")
end

local function build_shell_chain(cmds, stamp_path)
    local parts = {}
    for _, cmd in ipairs(cmds) do
        table.insert(parts, cmd)
    end
    if os.ishost("windows") then
        if stamp_path then
            table.insert(parts, 'type nul > "' .. stamp_path:gsub("/", "\\") .. '"')
        end
        return 'cmd /c "' .. table.concat(parts, " && ") .. '"'
    else
        if stamp_path then
            table.insert(parts, 'touch "' .. stamp_path .. '"')
        end
        local joined = table.concat(parts, " && ")
        return "sh -c '" .. joined:gsub("'", "'\\''") .. "'"
    end
end

local function get_defines_flags(cfg, is_msc)
    local out = {}
    for _, def in ipairs(cfg.defines or {}) do
        if is_msc then
            table.insert(out, format_define_msvc(def))
        else
            table.insert(out, format_define_posix(def))
        end
    end
    for _, undef in ipairs(cfg.undefines or {}) do
        if is_msc then
            table.insert(out, "/U" .. undef)
        else
            table.insert(out, "-U" .. undef)
        end
    end
    return out
end

local function get_include_flags(prj, cfg, is_msc)
    local out = {}
    local inc_flag = is_msc and "/I" or "-I"
    for _, dir in ipairs(cfg.includedirs or {}) do
        local rel = normalize_rel(prj, dir)
        table.insert(out, inc_flag .. quote_arg(rel))
    end
    for _, dir in ipairs(cfg.externalincludedirs or {}) do
        local rel = normalize_rel(prj, dir)
        if is_msc then
            table.insert(out, "/external:I" .. quote_arg(rel))
        else
            table.insert(out, "-isystem " .. quote_arg(rel))
        end
    end
    return out
end

local function get_link_args(prj, cfg, ts, is_msc)
    local sibling_paths = p.config.getlinks(cfg, "siblings", "fullpath") or {}
    local rel_siblings = {}
    for _, sp in ipairs(sibling_paths) do
        table.insert(rel_siblings, normalize_rel(prj, sp))
    end

    local sys_links = {}
    if ts.getlinks then
        sys_links = ts.getlinks(cfg, true, true) or {}
    else
        for _, l in ipairs(p.config.getlinks(cfg, "system", "basename") or {}) do
            table.insert(sys_links, is_msc and (l .. ".lib") or ("-l" .. l))
        end
    end

    local all_links = {}
    for _, s in ipairs(rel_siblings) do
        table.insert(all_links, quote_arg(s))
    end
    for _, l in ipairs(sys_links) do
        table.insert(all_links, l)
    end

    if not is_msc and cfg.linkgroups == p.ON and #all_links > 0 then
        table.insert(all_links, 1, "-Wl,--start-group")
        table.insert(all_links, "-Wl,--end-group")
    end

    return table.concat(all_links, " "), rel_siblings
end

function ninja.generateWorkspace(wks)
    p.w("# " .. wks.name .. " workspace Ninja build file generated by Premake")
    p.w("ninja_required_version = 1.6")
    p.w("")

    local projects = {}
    for prj in p.workspace.eachproject(wks) do
        table.insert(projects, prj)
        p.w("subninja " .. ninja_esc_path(prj.name .. ".ninja"))
    end
    p.w("")

    local start_prj = wks.startproject
    if not start_prj or start_prj == "" then
        start_prj = projects[1] and projects[1].name or nil
    end

    local first_cfg = nil
    local wks_cfgs = {}
    for cfg in p.workspace.eachconfig(wks) do
        table.insert(wks_cfgs, cfg)
        if not first_cfg then
            first_cfg = cfg
        end
    end

    if start_prj and first_cfg then
        p.w("# Default build target")
        p.w("default " .. sanitize_id(start_prj .. "_" .. first_cfg.shortname))
        p.w("")
    end

    p.w("# Workspace configuration targets")
    for _, wcfg in ipairs(wks_cfgs) do
        local targets = {}
        for _, prj in ipairs(projects) do
            if p.project.getconfig(prj, wcfg.buildcfg, wcfg.platform) then
                table.insert(targets, sanitize_id(prj.name .. "_" .. wcfg.shortname))
            end
        end
        if #targets > 0 then
            p.w("build " .. sanitize_id(wcfg.shortname) .. ": phony " .. table.concat(targets, " "))
        end
    end
    p.w("")
end

function ninja.generateProject(prj)
    p.w("# " .. prj.name .. " project Ninja build file generated by Premake")
    p.w("ninja_required_version = 1.6")
    p.w("")

    emit_rules(prj)

    local config_targets = {}
    local first_target = nil

    for cfg in p.project.eachconfig(prj) do
        local ts, base, is_msc = resolve_toolset(cfg)
        local tag = sanitize_id(base .. (cfg.gccprefix and cfg.gccprefix ~= "" and ("_" .. cfg.gccprefix) or ""))
        local cfg_id = sanitize_id(prj.name .. "_" .. cfg.shortname)

        local objdir = normalize_rel(prj, cfg.objdir)
        local target_rel = normalize_rel(prj, cfg.buildtarget.abspath)
        local targetdir = normalize_rel(prj, cfg.buildtarget.directory)
        local targetname = cfg.buildtarget.name

        local defs = get_defines_flags(cfg, is_msc)
        local incs = get_include_flags(prj, cfg, is_msc)
        local forceinc = (ts.getforceincludes and ts.getforceincludes(cfg)) or {}
        local base_cflags = (ts.getcflags and ts.getcflags(cfg)) or {}
        local base_cxxflags = (ts.getcxxflags and ts.getcxxflags(cfg)) or {}
        local extra_build = cfg.buildoptions or {}

        local cflags_str = join_flags({
            table.concat(base_cflags, " "),
            table.concat(defs, " "),
            table.concat(incs, " "),
            table.concat(forceinc, " "),
            table.concat(extra_build, " "),
        })

        local cxxflags_str = join_flags({
            is_msc and table.concat(base_cflags, " ") or "",
            table.concat(base_cxxflags, " "),
            table.concat(defs, " "),
            table.concat(incs, " "),
            table.concat(forceinc, " "),
            table.concat(extra_build, " "),
        })

        local base_ldflags = (ts.getldflags and ts.getldflags(cfg)) or {}
        local libdirs = (ts.getLibraryDirectories and ts.getLibraryDirectories(cfg)) or {}
        local extra_link = cfg.linkoptions or {}
        local ldflags_str = join_flags({
            table.concat(base_ldflags, " "),
            table.concat(extra_link, " "),
            table.concat(libdirs, " "),
        })

        local links_str, rel_siblings = get_link_args(prj, cfg, ts, is_msc)

        p.w("# Configuration: " .. cfg_id)
        p.w("")
        p.w("cflags_" .. cfg_id .. " = " .. ninja_esc_val(cflags_str))
        p.w("cxxflags_" .. cfg_id .. " = " .. ninja_esc_val(cxxflags_str))
        p.w("ldflags_" .. cfg_id .. " = " .. ninja_esc_val(ldflags_str))
        if cfg.kind ~= p.STATICLIB and links_str ~= "" then
            p.w("links_" .. cfg_id .. " = " .. ninja_esc_val(links_str))
        end
        p.w("objdir_" .. cfg_id .. " = " .. ninja_esc_val(objdir))
        p.w("targetdir_" .. cfg_id .. " = " .. ninja_esc_val(targetdir))
        p.w("target_" .. cfg_id .. " = " .. ninja_esc_val(targetname))
        p.w("")

        local prebuild_stamp = nil
        if cfg.prebuildcommands and #cfg.prebuildcommands > 0 then
            prebuild_stamp = objdir .. "/" .. prj.name .. ".prebuild"
            p.w("build " .. ninja_esc_path(prebuild_stamp) .. ": prebuild")
            p.w("  prebuildcommands = " .. ninja_esc_val(build_shell_chain(cfg.prebuildcommands, prebuild_stamp)))
        end

        local objects = {}
        local obj_ext = is_msc and ".obj" or ".o"
        local used_objs = {}

        local tr = p.project.getsourcetree(prj)
        p.tree.traverse(tr, {
            onleaf = function(node)
                local fcfg = p.fileconfig.getconfig(node, cfg)
                if not fcfg or fcfg.flags.ExcludeFromBuild then
                    return
                end

                local ext = path.getextension(node.abspath)
                local rule = nil
                local flag_var = nil
                if is_cxx_like(ext) then
                    rule = "cxx_" .. tag
                    flag_var = "cxxflags"
                elseif is_c_like(ext) then
                    rule = "cc_" .. tag
                    flag_var = "cflags"
                elseif is_rc(ext) then
                    rule = "rc_" .. tag
                    flag_var = "resflags"
                else
                    return
                end

                local base_obj = (fcfg.objname or node.objname or path.getbasename(node.abspath))
                local obj_name = base_obj
                if used_objs[obj_name] then
                    local idx = 1
                    while used_objs[base_obj .. tostring(idx)] do
                        idx = idx + 1
                    end
                    obj_name = base_obj .. tostring(idx)
                end
                used_objs[obj_name] = true

                local obj_path = objdir .. "/" .. obj_name .. (is_rc(ext) and ".res" or obj_ext)
                local src_rel = normalize_rel(prj, node.abspath)
                local implicit = prebuild_stamp and (" | " .. ninja_esc_path(prebuild_stamp)) or ""

                p.w("build " .. ninja_esc_path(obj_path) .. ": " .. rule .. " " .. ninja_esc_path(src_rel) .. implicit)
                if flag_var == "resflags" then
                    local resflags = join_flags({
                        table.concat(defs, " "),
                        table.concat(incs, " "),
                    })
                    p.w("  resflags = " .. ninja_esc_val(resflags))
                else
                    p.w("  " .. flag_var .. " = $" .. flag_var .. "_" .. cfg_id)
                end

                table.insert(objects, obj_path)
            end
        })

        local esc_objects = {}
        for _, o in ipairs(objects) do
            table.insert(esc_objects, ninja_esc_path(o))
        end

        local implicit_deps = {}
        for _, sib in ipairs(rel_siblings) do
            table.insert(implicit_deps, ninja_esc_path(sib))
        end

        if cfg.prelinkcommands and #cfg.prelinkcommands > 0 then
            local prelink_stamp = objdir .. "/" .. prj.name .. ".prelink"
            local prelink_inputs = #esc_objects > 0 and (" " .. table.concat(esc_objects, " ")) or ""
            p.w("build " .. ninja_esc_path(prelink_stamp) .. ": prelink" .. prelink_inputs)
            p.w("  prelinkcommands = " .. ninja_esc_val(build_shell_chain(cfg.prelinkcommands, prelink_stamp)))
            table.insert(implicit_deps, ninja_esc_path(prelink_stamp))
        end

        local link_rule = (cfg.kind == p.STATICLIB) and ("ar_" .. tag) or ("link_" .. tag)
        local imp_str = #implicit_deps > 0 and (" | " .. table.concat(implicit_deps, " ")) or ""
        p.w("build " .. ninja_esc_path(target_rel) .. ": " .. link_rule .. " " .. table.concat(esc_objects, " ") .. imp_str)
        if cfg.kind ~= p.STATICLIB then
            p.w("  ldflags = $ldflags_" .. cfg_id)
            if links_str ~= "" then
                p.w("  links = $links_" .. cfg_id)
            end
        end

        local final_target = target_rel
        if cfg.postbuildcommands and #cfg.postbuildcommands > 0 then
            local postbuild_stamp = objdir .. "/" .. prj.name .. ".postbuild"
            p.w("build " .. ninja_esc_path(postbuild_stamp) .. ": postbuild " .. ninja_esc_path(target_rel))
            p.w("  postbuildcommands = " .. ninja_esc_val(build_shell_chain(cfg.postbuildcommands, postbuild_stamp)))
            final_target = postbuild_stamp
        end
        p.w("")

        table.insert(config_targets, { id = cfg_id, target = final_target })
        if not first_target then
            first_target = final_target
        end
    end

    p.w("# Build targets")
    p.w("")
    for _, ct in ipairs(config_targets) do
        p.w("build " .. ct.id .. ": phony " .. ninja_esc_path(ct.target))
    end
    p.w("")

    if first_target then
        p.w("# Project phony targets")
        p.w("")
        p.w("build " .. sanitize_id(prj.name) .. ": phony " .. ninja_esc_path(first_target))
    end
    p.w("")
end

newaction {
    trigger = "ninja",
    shortname = "Ninja",
    description = "Generate Ninja build files",
    toolset = "gcc",

    valid_kinds = { "ConsoleApp", "WindowedApp", "StaticLib", "SharedLib", "Utility" },
    valid_languages = { "C", "C++" },
    valid_tools = {
        cc = { "gcc", "clang", "msc" },
    },

    onWorkspace = function(wks)
        p.generate(wks, "build.ninja", ninja.generateWorkspace)
    end,

    onProject = function(prj)
        p.generate(prj, ".ninja", ninja.generateProject)
    end,
}
