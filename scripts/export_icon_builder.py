#!/usr/bin/env python3

from pathlib import Path

import methods
import utils


def export_icon_builder(target, source):
    src_path = Path(str(source[0]))
    src_name = src_path.stem
    platform = src_path.parent.parent.stem

    with open(str(source[0]), "r", encoding="utf-8") as file:
        svg = file.read()

    with methods.generated_wrapper(str(target[0])) as file:
        file.write(f"""\
inline constexpr const char *_{platform}_{src_name}_svg = {methods.to_raw_cstring(svg)};
""")


run = utils.get_run_arg()
match run:
    case 0:
        export_icon_builder(
            ["platform/android/export/logo_svg.gen.h"],
            ["platform/android/export/logo.svg"],
        )
    case 1:
        export_icon_builder(
            ["platform/android/export/run_icon_svg.gen.h"],
            ["platform/android/export/run_icon.svg"],
        )
    case 2:
        export_icon_builder(
            ["platform/ios/export/logo_svg.gen.h"], ["platform/ios/export/logo.svg"]
        )
    case 3:
        export_icon_builder(
            ["platform/ios/export/run_icon_svg.gen.h"],
            ["platform/ios/export/run_icon.svg"],
        )
    case 4:
        export_icon_builder(
            ["platform/linuxbsd/export/logo_svg.gen.h"],
            ["platform/linuxbsd/export/logo.svg"],
        )
    case 5:
        export_icon_builder(
            ["platform/linuxbsd/export/run_icon_svg.gen.h"],
            ["platform/linuxbsd/export/run_icon.svg"],
        )
    case 6:
        export_icon_builder(
            ["platform/macos/export/logo_svg.gen.h"], ["platform/macos/export/logo.svg"]
        )
    case 7:
        export_icon_builder(
            ["platform/macos/export/run_icon_svg.gen.h"],
            ["platform/macos/export/run_icon.svg"],
        )
    case 8:
        export_icon_builder(
            ["platform/visionos/export/logo_svg.gen.h"],
            ["platform/visionos/export/logo.svg"],
        )
    case 9:
        export_icon_builder(
            ["platform/visionos/export/run_icon_svg.gen.h"],
            ["platform/visionos/export/run_icon.svg"],
        )
    case 10:
        export_icon_builder(
            ["platform/web/export/logo_svg.gen.h"], ["platform/web/export/logo.svg"]
        )
    case 11:
        export_icon_builder(
            ["platform/web/export/run_icon_svg.gen.h"],
            ["platform/web/export/run_icon.svg"],
        )
    case 12:
        export_icon_builder(
            ["platform/windows/export/logo_svg.gen.h"],
            ["platform/windows/export/logo.svg"],
        )
    case 13:
        export_icon_builder(
            ["platform/windows/export/run_icon_svg.gen.h"],
            ["platform/windows/export/run_icon.svg"],
        )
    case _:
        print("No commands left to run.")
