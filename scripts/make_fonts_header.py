#!/usr/bin/env python3

"""Functions used to generate source files during build time"""

import os
import utils

import methods


def make_fonts_header(target, source):
    with methods.generated_wrapper(str(target[0])) as file:
        for src in map(str, source):
            # Saving uncompressed, since FreeType will reference from memory pointer.
            buffer = methods.get_buffer(src)
            name = os.path.splitext(os.path.basename(src))[0]

            file.write(f"""\
inline constexpr int _font_{name}_size = {len(buffer)};
inline constexpr unsigned char _font_{name}[] = {{
	{methods.format_buffer(buffer, 1)}
}};

""")

run = utils.get_run_arg()
match run:
    case 0:
        make_fonts_header(["editor/themes/builtin_fonts.gen.h"], ["thirdparty/fonts/DroidSansFallback.woff2", "thirdparty/fonts/DroidSansJapanese.woff2", "thirdparty/fonts/Inter_Bold.woff2", "thirdparty/fonts/Inter_Regular.woff2", "thirdparty/fonts/JetBrainsMono_Regular.woff2", "thirdparty/fonts/NotoSansBengali_Bold.woff2", "thirdparty/fonts/NotoSansBengali_Regular.woff2", "thirdparty/fonts/NotoSansDevanagari_Bold.woff2", "thirdparty/fonts/NotoSansDevanagari_Regular.woff2", "thirdparty/fonts/NotoSansGeorgian_Bold.woff2", "thirdparty/fonts/NotoSansGeorgian_Regular.woff2", "thirdparty/fonts/NotoSansHebrew_Bold.woff2", "thirdparty/fonts/NotoSansHebrew_Regular.woff2", "thirdparty/fonts/NotoSansMalayalamUI_Bold.woff2", "thirdparty/fonts/NotoSansMalayalamUI_Regular.woff2", "thirdparty/fonts/NotoSansOriya_Bold.woff2", "thirdparty/fonts/NotoSansOriya_Regular.woff2", "thirdparty/fonts/NotoSansSinhala_Bold.woff2", "thirdparty/fonts/NotoSansSinhala_Regular.woff2", "thirdparty/fonts/NotoSansTamilUI_Bold.woff2", "thirdparty/fonts/NotoSansTamilUI_Regular.woff2", "thirdparty/fonts/NotoSansTeluguUI_Bold.woff2", "thirdparty/fonts/NotoSansTeluguUI_Regular.woff2", "thirdparty/fonts/NotoSansThai_Bold.woff2", "thirdparty/fonts/NotoSansThai_Regular.woff2", "thirdparty/fonts/OpenSans_SemiBold.woff2", "thirdparty/fonts/Vazirmatn_Bold.woff2", "thirdparty/fonts/Vazirmatn_Regular.woff2"])
    case 1:
        make_fonts_header(["scene/theme/default_font.gen.h"], ["thirdparty/fonts/OpenSans_SemiBold.woff2"])
    case _:
        print("No commands left to run.")
