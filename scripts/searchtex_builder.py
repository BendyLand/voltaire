#!/usr/bin/env python3

import methods


def searchtex_builder(target, source):
    buffer = methods.get_buffer(str(source[0]))

    with methods.generated_wrapper(str(target[0])) as file:
        file.write(f"""\
#define SEARCHTEX_WIDTH 64
#define SEARCHTEX_HEIGHT 16
#define SEARCHTEX_PITCH SEARCHTEX_WIDTH
#define SEARCHTEX_SIZE (SEARCHTEX_HEIGHT * SEARCHTEX_PITCH)

inline constexpr const unsigned char search_tex_png[] = {{
{methods.format_buffer(buffer, 1)}
}};
""")

if __name__ == "__main__":
    searchtex_builder(
        ["servers/rendering/renderer_rd/effects/smaa_search_tex.gen.h"],
        ["thirdparty/smaa/SearchTex.png"],
    )
