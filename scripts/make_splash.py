#!/usr/bin/env python3

import os
from collections import OrderedDict
from pathlib import Path

import methods

EXPORT_ICONS = [
    "platform/android/export/logo.svg",
    "platform/android/export/run_icon.svg",
    "platform/ios/export/logo.svg",
    "platform/ios/export/run_icon.svg",
    "platform/linuxbsd/export/logo.svg",
    "platform/linuxbsd/export/run_icon.svg",
    "platform/macos/export/logo.svg",
    "platform/macos/export/run_icon.svg",
    "platform/visionos/export/logo.svg",
    "platform/visionos/export/run_icon.svg",
    "platform/web/export/logo.svg",
    "platform/web/export/run_icon.svg",
    "platform/windows/export/logo.svg",
    "platform/windows/export/run_icon.svg",
]

THEME_ICONS = [
    "scene/theme/icons/action_copy.svg", "scene/theme/icons/add.svg", "scene/theme/icons/arrow_down.svg",
    "scene/theme/icons/arrow_left.svg", "scene/theme/icons/arrow_right.svg", "scene/theme/icons/arrow_up.svg",
    "scene/theme/icons/bookmark.svg", "scene/theme/icons/breakpoint.svg", "scene/theme/icons/checked.svg",
    "scene/theme/icons/checked_disabled.svg", "scene/theme/icons/clear.svg", "scene/theme/icons/close.svg",
    "scene/theme/icons/close_hl.svg", "scene/theme/icons/color_picker_bar_arrow.svg", "scene/theme/icons/color_picker_cursor.svg",
    "scene/theme/icons/color_picker_cursor_bg.svg", "scene/theme/icons/color_picker_overbright.svg", "scene/theme/icons/color_picker_pipette.svg",
    "scene/theme/icons/error_icon.svg", "scene/theme/icons/favorite.svg", "scene/theme/icons/file.svg",
    "scene/theme/icons/file_mode_list.svg", "scene/theme/icons/file_mode_thumbnail.svg", "scene/theme/icons/file_thumbnail.svg",
    "scene/theme/icons/folder.svg", "scene/theme/icons/folder_create.svg", "scene/theme/icons/folder_thumbnail.svg",
    "scene/theme/icons/folder_up.svg", "scene/theme/icons/graph_port.svg", "scene/theme/icons/grid_layout.svg",
    "scene/theme/icons/grid_minimap.svg", "scene/theme/icons/grid_snap.svg", "scene/theme/icons/grid_toggle.svg",
    "scene/theme/icons/h_dragger.svg", "scene/theme/icons/hslider_tick.svg", "scene/theme/icons/hsplitter.svg",
    "scene/theme/icons/indeterminate.svg", "scene/theme/icons/indeterminate_disabled.svg", "scene/theme/icons/line_edit_clear.svg",
    "scene/theme/icons/load.svg", "scene/theme/icons/mini_checkerboard.svg", "scene/theme/icons/move_down.svg",
    "scene/theme/icons/move_up.svg", "scene/theme/icons/option_button_arrow.svg", "scene/theme/icons/picker_shape_circle.svg",
    "scene/theme/icons/picker_shape_rectangle.svg", "scene/theme/icons/picker_shape_rectangle_wheel.svg", "scene/theme/icons/popup_menu_arrow_left.svg",
    "scene/theme/icons/popup_menu_arrow_right.svg", "scene/theme/icons/radio_checked.svg", "scene/theme/icons/radio_checked_disabled.svg",
    "scene/theme/icons/radio_unchecked.svg", "scene/theme/icons/radio_unchecked_disabled.svg", "scene/theme/icons/region_folded.svg",
    "scene/theme/icons/region_unfolded.svg", "scene/theme/icons/reload.svg", "scene/theme/icons/resizer_nw.svg",
    "scene/theme/icons/resizer_se.svg", "scene/theme/icons/save.svg", "scene/theme/icons/script.svg",
    "scene/theme/icons/scroll_button_left.svg", "scene/theme/icons/scroll_button_left_hl.svg", "scene/theme/icons/scroll_button_right.svg",
    "scene/theme/icons/scroll_button_right_hl.svg", "scene/theme/icons/scroll_hint_horizontal.svg", "scene/theme/icons/scroll_hint_vertical.svg",
    "scene/theme/icons/search.svg", "scene/theme/icons/slider_grabber.svg", "scene/theme/icons/slider_grabber_disabled.svg",
    "scene/theme/icons/slider_grabber_hl.svg", "scene/theme/icons/sort.svg", "scene/theme/icons/tabs_drop_mark.svg",
    "scene/theme/icons/tabs_menu.svg", "scene/theme/icons/tabs_menu_hl.svg", "scene/theme/icons/text_edit_ellipsis.svg",
    "scene/theme/icons/text_edit_space.svg", "scene/theme/icons/text_edit_tab.svg", "scene/theme/icons/toggle_filename_filter.svg",
    "scene/theme/icons/toggle_off.svg", "scene/theme/icons/toggle_off_disabled.svg", "scene/theme/icons/toggle_off_disabled_mirrored.svg",
    "scene/theme/icons/toggle_off_mirrored.svg", "scene/theme/icons/toggle_on.svg", "scene/theme/icons/toggle_on_disabled.svg",
    "scene/theme/icons/toggle_on_disabled_mirrored.svg", "scene/theme/icons/toggle_on_mirrored.svg", "scene/theme/icons/unchecked.svg",
    "scene/theme/icons/unchecked_disabled.svg", "scene/theme/icons/updown.svg", "scene/theme/icons/v_dragger.svg",
    "scene/theme/icons/value_down.svg", "scene/theme/icons/value_up.svg", "scene/theme/icons/visibility_visible.svg",
    "scene/theme/icons/vslider_tick.svg", "scene/theme/icons/vsplitter.svg", "scene/theme/icons/zoom_less.svg",
    "scene/theme/icons/zoom_more.svg", "scene/theme/icons/zoom_reset.svg",
]


def make_splash_and_icons(target, splash_src, icon_src):
    splash_buf = methods.get_buffer(str(splash_src))
    icon_buf = methods.get_buffer(str(icon_src))
    font_buf = methods.get_buffer("thirdparty/fonts/OpenSans_SemiBold.woff2")

    icons_names = []
    icons_raw = []
    for src in THEME_ICONS:
        with open(src, encoding="utf-8", newline="\n") as f:
            icons_raw.append(methods.to_raw_cstring(f.read()))
        icons_names.append(f'"{os.path.splitext(os.path.basename(src))[0]}"')

    icons_names_str = ",\n\t".join(icons_names)
    icons_raw_str = ",\n\t".join(icons_raw)

    platforms = ["android", "ios", "visionos", "web"]
    api_reg = "\n\t".join([f"register_{p}_api();" for p in platforms])
    api_unreg = "\n\t".join([f"unregister_{p}_api();" for p in platforms])

    modules = OrderedDict(methods.detect_modules("modules", recursive=False).items())
    decls = []
    for key in modules.keys():
        if key not in ("freetype", "msdfgen"):
            decls.append(f"void initialize_{key}_module(ModuleInitializationLevel p_level);")
            decls.append(f"void uninitialize_{key}_module(ModuleInitializationLevel p_level);")
    mod_inc = "\n".join(decls)
    mod_init = "\n".join([f"#ifdef MODULE_{key.upper()}_ENABLED\n\tinitialize_{key}_module(p_level);\n#endif" for key in modules.keys()])
    mod_uninit = "\n".join([f"#ifdef MODULE_{key.upper()}_ENABLED\n\tuninitialize_{key}_module(p_level);\n#endif" for key in modules.keys()])

    with open(str(target), "w", encoding="utf-8", newline="\n") as out:
        out.write("/* THIS FILE IS GENERATED. EDITS WILL BE LOST. */\n\n")
        out.write("#ifndef MAIN_SPLASH_GEN_H\n#define MAIN_SPLASH_GEN_H\n\n")
        out.write(f"""\
#include "core/math/vector2.h"

inline const Color boot_splash_bg_color = Color(0.14, 0.14, 0.14);
inline constexpr const unsigned char boot_splash_png[] = {{
{methods.format_buffer(splash_buf, 1)}
}};

inline constexpr const unsigned char app_icon_png[] = {{
{methods.format_buffer(icon_buf, 1)}
}};

#ifndef _FONT_OPENSANS_SEMIBOLD_DEFINED
#define _FONT_OPENSANS_SEMIBOLD_DEFINED
inline constexpr int _font_OpenSans_SemiBold_size = {len(font_buf)};
inline constexpr unsigned char _font_OpenSans_SemiBold[] = {{
	{methods.format_buffer(font_buf, 1)}
}};
#endif // _FONT_OPENSANS_SEMIBOLD_DEFINED

inline constexpr int default_theme_icons_count = {len(icons_names)};
inline constexpr const char *default_theme_icons_sources[] = {{
	{icons_raw_str}
}};

inline constexpr const char *default_theme_icons_names[] = {{
	{icons_names_str}
}};

""")
        for src in EXPORT_ICONS:
            src_path = Path(src)
            src_name = src_path.stem
            platform = src_path.parent.parent.stem
            with open(src, "r", encoding="utf-8") as f:
                svg = f.read()
            out.write(f"inline constexpr const char *_{platform}_{src_name}_svg = {methods.to_raw_cstring(svg)};\n")

        out.write(f"""
#endif // MAIN_SPLASH_GEN_H

#ifdef DEFINE_MAIN_GENERATED_REGISTRIES
#undef DEFINE_MAIN_GENERATED_REGISTRIES

#include "modules/modules_enabled.gen.h"

void register_platform_apis() {{
	{api_reg}
}}

void unregister_platform_apis() {{
	{api_unreg}
}}

{mod_inc}

void initialize_modules(ModuleInitializationLevel p_level) {{
{mod_init}
}}

void uninitialize_modules(ModuleInitializationLevel p_level) {{
{mod_uninit}
}}

#endif // DEFINE_MAIN_GENERATED_REGISTRIES
""")


if __name__ == "__main__":
    make_splash_and_icons("main/splash.gen.h", "main/splash.png", "main/app_icon.png")
