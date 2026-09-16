#!/usr/bin/env python3

"""Functions used to generate source files during build time"""

import os

import methods

def make_editor_icons_action(target, source):
    out_path = str(target[0])
    out_dir = os.path.dirname(out_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    icons_names = []
    icons_raw = []
    icons_med = []
    icons_big = []

    valid_sources = [src for src in source if os.path.isfile(str(src))]

    for idx, svg in enumerate(valid_sources):
        path = str(svg)
        with open(path, encoding="utf-8", newline="\n") as file:
            icons_raw.append(methods.to_raw_cstring(file.read()))

        name = os.path.splitext(os.path.basename(path))[0]
        icons_names.append(f'"{name}"')

        if name.endswith("MediumThumb"):
            icons_med.append(str(idx))
        elif name.endswith(("BigThumb", "GodotFile")):
            icons_big.append(str(idx))

    icons_names_str = ",\n\t".join(icons_names)
    icons_raw_str = ",\n\t".join(icons_raw)

    with methods.generated_wrapper(out_path) as file:
        file.write(f"""\
inline constexpr int editor_icons_count = {len(icons_names)};
inline constexpr const char *editor_icons_sources[] = {{
\t{icons_raw_str}
}};

inline constexpr const char *editor_icons_names[] = {{
\t{icons_names_str}
}};

inline constexpr int editor_md_thumbs_count = {len(icons_med)};
inline constexpr int editor_md_thumbs_indices[] = {{ {", ".join(icons_med)} }};

inline constexpr int editor_bg_thumbs_count = {len(icons_big)};
inline constexpr int editor_bg_thumbs_indices[] = {{ {", ".join(icons_big)} }};
""")
