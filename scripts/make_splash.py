#!/usr/bin/env python3

import methods


def make_splash_and_icon(target, splash_src, icon_src):
    splash_buf = methods.get_buffer(str(splash_src))
    icon_buf = methods.get_buffer(str(icon_src))

    with methods.generated_wrapper(str(target)) as file:
        file.write(f"""\
#include "core/math/color.h"

static const Color boot_splash_bg_color = Color(0.14, 0.14, 0.14);
inline constexpr const unsigned char boot_splash_png[] = {{
{methods.format_buffer(splash_buf, 1)}
}};

inline constexpr const unsigned char app_icon_png[] = {{
{methods.format_buffer(icon_buf, 1)}
}};
""")


if __name__ == "__main__":
    make_splash_and_icon("main/splash.gen.h", "main/splash.png", "main/app_icon.png")
