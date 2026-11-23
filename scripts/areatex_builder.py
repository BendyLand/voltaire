#!/usr/bin/env python3

import methods


def smaa_tex_builder(target, area_src, search_src):
    area_buf = methods.get_buffer(str(area_src))
    search_buf = methods.get_buffer(str(search_src))

    with methods.generated_wrapper(str(target)) as file:
        file.write(f"""\
#define AREATEX_WIDTH 160
#define AREATEX_HEIGHT 560
#define AREATEX_PITCH (AREATEX_WIDTH * 2)
#define AREATEX_SIZE (AREATEX_HEIGHT * AREATEX_PITCH)

inline constexpr const unsigned char area_tex_png[] = {{
{methods.format_buffer(area_buf, 1)}
}};

#define SEARCHTEX_WIDTH 64
#define SEARCHTEX_HEIGHT 16
#define SEARCHTEX_PITCH SEARCHTEX_WIDTH
#define SEARCHTEX_SIZE (SEARCHTEX_HEIGHT * SEARCHTEX_PITCH)

inline constexpr const unsigned char search_tex_png[] = {{
{methods.format_buffer(search_buf, 1)}
}};
""")


if __name__ == "__main__":
    smaa_tex_builder(
        "servers/rendering/renderer_rd/effects/smaa_area_tex.gen.h",
        "thirdparty/smaa/AreaTex.png",
        "thirdparty/smaa/SearchTex.png",
    )
