#!/usr/bin/env python3

import os
from collections import OrderedDict

import methods


def register_module_types_builder(target, source):
    modules = OrderedDict(source[0].items())
    decls = []
    for key, val in modules.items():
        rt_hdr = f"{val}/register_types.h"
        if os.path.exists(rt_hdr):
            decls.append(f'#include "{rt_hdr}"')
        elif key not in ("freetype", "msdfgen"):
            decls.append(f"void initialize_{key}_module(ModuleInitializationLevel p_level);")
            decls.append(f"void uninitialize_{key}_module(ModuleInitializationLevel p_level);")
    mod_inc = "\n".join(decls)
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
        methods.detect_modules("modules", recursive=False),
        "modules/modules_enabled.gen.h",
    ],
)
