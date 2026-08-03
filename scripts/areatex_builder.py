#!/usr/bin/env python3

import methods


def areatex_builder(target, source):
    buffer = methods.get_buffer(str(source[0]))

    with methods.generated_wrapper(str(target[0])) as file:
        file.write(f"""\
#define AREATEX_WIDTH 160
#define AREATEX_HEIGHT 560
#define AREATEX_PITCH (AREATEX_WIDTH * 2)
#define AREATEX_SIZE (AREATEX_HEIGHT * AREATEX_PITCH)

inline constexpr const unsigned char area_tex_png[] = {{
{methods.format_buffer(buffer, 1)}
}};
""")

if __name__ == "__main__":
    areatex_builder(
        ["servers/rendering/renderer_rd/effects/smaa_area_tex.gen.h"],
        ["thirdparty/smaa/AreaTex.png"],
    )
