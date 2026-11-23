#!/usr/bin/env python3

import methods


def register_exporters_builder(target, source):
    platforms = source[0]
    exp_reg = "\n\t".join([f"register_{p}_exporter();" for p in platforms])
    exp_type = "\n\t".join([f"register_{p}_exporter_types();" for p in platforms])
    with methods.generated_wrapper(str(target[0])) as file:
        file.write(f"""\
#include "editor/editor_ui.h"

void register_exporters() {{
	{exp_reg}
}}

void register_exporter_types() {{
	{exp_type}
}}
""")


register_exporters_builder(
    ["editor/export/register_exporters.gen.cpp"],
    [["android", "ios", "linuxbsd", "macos", "visionos", "web", "windows"]],
)
