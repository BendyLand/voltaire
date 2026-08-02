#!/usr/bin/env python3

import methods


def make_splash(target, source):
    buffer = methods.get_buffer(str(source[0]))

    with methods.generated_wrapper(str(target[0])) as file:
        # Use a neutral gray color to better fit various kinds of projects.
        file.write(f"""\
#include "core/math/color.h"

static const Color boot_splash_bg_color = Color(0.14, 0.14, 0.14);
inline constexpr const unsigned char boot_splash_png[] = {{
{methods.format_buffer(buffer, 1)}
}};
""")

make_splash(["main/splash.gen.h"], ["main/splash.png"])
