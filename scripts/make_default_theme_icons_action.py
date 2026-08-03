#!/usr/bin/env python3

"""Functions used to generate source files during build time"""

import os

import methods


# See also `editor/icons/editor_icons_builders.py`.
def make_default_theme_icons_action(target, source):
    icons_names = []
    icons_raw = []

    for src in map(str, source):
        with open(src, encoding="utf-8", newline="\n") as file:
            icons_raw.append(methods.to_raw_cstring(file.read()))

        name = os.path.splitext(os.path.basename(src))[0]
        icons_names.append(f'"{name}"')

    icons_names_str = ",\n\t".join(icons_names)
    icons_raw_str = ",\n\t".join(icons_raw)

    with methods.generated_wrapper(str(target[0])) as file:
        file.write(f"""\
inline constexpr int default_theme_icons_count = {len(icons_names)};
inline constexpr const char *default_theme_icons_sources[] = {{
	{icons_raw_str}
}};

inline constexpr const char *default_theme_icons_names[] = {{
	{icons_names_str}
}};
""")

make_default_theme_icons_action(["scene/theme/default_theme_icons.gen.h"], ["scene/theme/icons/action_copy.svg", "scene/theme/icons/add.svg", "scene/theme/icons/arrow_down.svg", "scene/theme/icons/arrow_left.svg", "scene/theme/icons/arrow_right.svg", "scene/theme/icons/arrow_up.svg", "scene/theme/icons/bookmark.svg", "scene/theme/icons/breakpoint.svg", "scene/theme/icons/checked.svg", "scene/theme/icons/checked_disabled.svg", "scene/theme/icons/clear.svg", "scene/theme/icons/close.svg", "scene/theme/icons/close_hl.svg", "scene/theme/icons/color_picker_bar_arrow.svg", "scene/theme/icons/color_picker_cursor.svg", "scene/theme/icons/color_picker_cursor_bg.svg", "scene/theme/icons/color_picker_overbright.svg", "scene/theme/icons/color_picker_pipette.svg", "scene/theme/icons/error_icon.svg", "scene/theme/icons/favorite.svg", "scene/theme/icons/file.svg", "scene/theme/icons/file_mode_list.svg", "scene/theme/icons/file_mode_thumbnail.svg", "scene/theme/icons/file_thumbnail.svg", "scene/theme/icons/folder.svg", "scene/theme/icons/folder_create.svg", "scene/theme/icons/folder_thumbnail.svg", "scene/theme/icons/folder_up.svg", "scene/theme/icons/graph_port.svg", "scene/theme/icons/grid_layout.svg", "scene/theme/icons/grid_minimap.svg", "scene/theme/icons/grid_snap.svg", "scene/theme/icons/grid_toggle.svg", "scene/theme/icons/h_dragger.svg", "scene/theme/icons/hslider_tick.svg", "scene/theme/icons/hsplitter.svg", "scene/theme/icons/indeterminate.svg", "scene/theme/icons/indeterminate_disabled.svg", "scene/theme/icons/line_edit_clear.svg", "scene/theme/icons/load.svg", "scene/theme/icons/mini_checkerboard.svg", "scene/theme/icons/move_down.svg", "scene/theme/icons/move_up.svg", "scene/theme/icons/option_button_arrow.svg", "scene/theme/icons/picker_shape_circle.svg", "scene/theme/icons/picker_shape_rectangle.svg", "scene/theme/icons/picker_shape_rectangle_wheel.svg", "scene/theme/icons/popup_menu_arrow_left.svg", "scene/theme/icons/popup_menu_arrow_right.svg", "scene/theme/icons/radio_checked.svg", "scene/theme/icons/radio_checked_disabled.svg", "scene/theme/icons/radio_unchecked.svg", "scene/theme/icons/radio_unchecked_disabled.svg", "scene/theme/icons/region_folded.svg", "scene/theme/icons/region_unfolded.svg", "scene/theme/icons/reload.svg", "scene/theme/icons/resizer_nw.svg", "scene/theme/icons/resizer_se.svg", "scene/theme/icons/save.svg", "scene/theme/icons/script.svg", "scene/theme/icons/scroll_button_left.svg", "scene/theme/icons/scroll_button_left_hl.svg", "scene/theme/icons/scroll_button_right.svg", "scene/theme/icons/scroll_button_right_hl.svg", "scene/theme/icons/scroll_hint_horizontal.svg", "scene/theme/icons/scroll_hint_vertical.svg", "scene/theme/icons/search.svg", "scene/theme/icons/slider_grabber.svg", "scene/theme/icons/slider_grabber_disabled.svg", "scene/theme/icons/slider_grabber_hl.svg", "scene/theme/icons/sort.svg", "scene/theme/icons/tabs_drop_mark.svg", "scene/theme/icons/tabs_menu.svg", "scene/theme/icons/tabs_menu_hl.svg", "scene/theme/icons/text_edit_ellipsis.svg", "scene/theme/icons/text_edit_space.svg", "scene/theme/icons/text_edit_tab.svg", "scene/theme/icons/toggle_filename_filter.svg", "scene/theme/icons/toggle_off.svg", "scene/theme/icons/toggle_off_disabled.svg", "scene/theme/icons/toggle_off_disabled_mirrored.svg", "scene/theme/icons/toggle_off_mirrored.svg", "scene/theme/icons/toggle_on.svg", "scene/theme/icons/toggle_on_disabled.svg", "scene/theme/icons/toggle_on_disabled_mirrored.svg", "scene/theme/icons/toggle_on_mirrored.svg", "scene/theme/icons/unchecked.svg", "scene/theme/icons/unchecked_disabled.svg", "scene/theme/icons/updown.svg", "scene/theme/icons/v_dragger.svg", "scene/theme/icons/value_down.svg", "scene/theme/icons/value_up.svg", "scene/theme/icons/visibility_visible.svg", "scene/theme/icons/vslider_tick.svg", "scene/theme/icons/vsplitter.svg", "scene/theme/icons/zoom_less.svg", "scene/theme/icons/zoom_more.svg", "scene/theme/icons/zoom_reset.svg"])

