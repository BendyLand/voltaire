#!/usr/bin/env python3

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


def export_icons_bundle_builder(target, sources):
    with methods.generated_wrapper(str(target)) as out:
        for src in sources:
            src_path = Path(src)
            src_name = src_path.stem
            platform = src_path.parent.parent.stem
            with open(src, "r", encoding="utf-8") as f:
                svg = f.read()
            out.write(f"inline constexpr const char *_{platform}_{src_name}_svg = {methods.to_raw_cstring(svg)};\n")


if __name__ == "__main__":
    export_icons_bundle_builder("platform/android/export/export_icons_svg.gen.h", EXPORT_ICONS)
