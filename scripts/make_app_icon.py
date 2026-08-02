#!/usr/bin/env python3

import methods


def make_app_icon(target, source):
    buffer = methods.get_buffer(str(source[0]))

    with methods.generated_wrapper(str(target[0])) as file:
        # Use a neutral gray color to better fit various kinds of projects.
        file.write(f"""\
inline constexpr const unsigned char app_icon_png[] = {{
{methods.format_buffer(buffer, 1)}
}};
""")

make_app_icon(["main/app_icon.gen.h"], ["main/app_icon.png"])

