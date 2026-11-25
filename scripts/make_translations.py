#!/usr/bin/env python3

import os
import os.path
import shutil
import subprocess
import sys
import tempfile
import uuid

import methods


def _compile_translation_entries(category, source):
    valid_sources = [src for src in source if os.path.isfile(src)]
    sorted_paths = sorted(
        [os.path.abspath(src) for src in valid_sources],
        key=lambda path: os.path.splitext(os.path.basename(path))[0],
    )
    xl_names = []
    blobs = []
    msgfmt = shutil.which("msgfmt")
    if not msgfmt:
        methods.print_warning("msgfmt not found, using .po files instead of .mo")

    for path in sorted_paths:
        name = os.path.splitext(os.path.basename(path))[0]
        if msgfmt and name != category:
            mo_path = os.path.join(tempfile.gettempdir(), uuid.uuid4().hex + ".mo")
            cmd = f'{msgfmt} "{path}" --no-hash -o "{mo_path}"'
            try:
                subprocess.Popen(cmd, shell=True, stderr=subprocess.PIPE).communicate()
                buffer = methods.get_buffer(mo_path)
            except OSError as e:
                methods.print_warning(
                    "msgfmt execution failed, using .po file instead of .mo: path=%r; [%s] %s"
                    % (path, e.__class__.__name__, e)
                )
                buffer = methods.get_buffer(path)
            finally:
                try:
                    if os.path.exists(mo_path):
                        os.remove(mo_path)
                except OSError as e:
                    methods.print_warning(
                        "Could not delete temporary .mo file: path=%r; [%s] %s"
                        % (mo_path, e.__class__.__name__, e)
                    )
        else:
            buffer = methods.get_buffer(path)
            if name == category:
                name = "source"

        decomp_size = len(buffer)
        buffer = methods.compress_buffer(buffer)
        blobs.append(
            f"inline constexpr const unsigned char _{category}_translation_{name}_compressed[] = {{\n"
            f"{methods.format_buffer(buffer, 1)}\n}};\n\n"
        )
        xl_names.append([name, len(buffer), decomp_size])

    return blobs, xl_names


def make_translations(target, source):
    target_h, target_cpp = str(target[0]), str(target[1])
    category = os.path.basename(target_cpp).split("_")[0]
    blobs, xl_names = _compile_translation_entries(category, source)

    with methods.generated_wrapper(target_cpp) as file:
        for b in blobs:
            file.write(b)
        file.write(f'#include "{target_h}"\n\nconst EditorTranslationList _{category}_translations[] = {{\n')
        for x in xl_names:
            file.write(f'\t{{ "{x[0]}", {x[1]}, {x[2]}, _{category}_translation_{x[0]}_compressed }},\n')
        file.write("\t{ nullptr, 0, 0, nullptr },\n};\n")

    with methods.generated_wrapper(target_h) as file:
        file.write(f"""
#ifndef EDITOR_TRANSLATION_LIST
#define EDITOR_TRANSLATION_LIST

struct EditorTranslationList {{
\tconst char* lang;
\tint comp_size;
\tint uncomp_size;
\tconst unsigned char* data;
}};

#endif // EDITOR_TRANSLATION_LIST

extern const EditorTranslationList _{category}_translations[];
""")


if __name__ == "__main__":
    run = int(sys.argv[1]) if len(sys.argv) > 1 else -1
    match run:
        case 0:
            make_translations(
                ["editor/translations/editor_translations.gen.h", "editor/translations/editor_translations.gen.cpp"],
                ["editor/translations/editor/ar.po", "editor/translations/editor/bg.po", "editor/translations/editor/bn.po", "editor/translations/editor/ca.po", "editor/translations/editor/cs.po", "editor/translations/editor/de.po", "editor/translations/editor/el.po", "editor/translations/editor/eo.po", "editor/translations/editor/es.po", "editor/translations/editor/es_AR.po", "editor/translations/editor/et.po", "editor/translations/editor/fa.po", "editor/translations/editor/fi.po", "editor/translations/editor/fr.po", "editor/translations/editor/ga.po", "editor/translations/editor/gl.po", "editor/translations/editor/he.po", "editor/translations/editor/hu.po", "editor/translations/editor/id.po", "editor/translations/editor/it.po", "editor/translations/editor/ja.po", "editor/translations/editor/ka.po", "editor/translations/editor/ko.po", "editor/translations/editor/nl.po", "editor/translations/editor/pl.po", "editor/translations/editor/pt.po", "editor/translations/editor/pt_BR.po", "editor/translations/editor/ro.po", "editor/translations/editor/ru.po", "editor/translations/editor/sk.po", "editor/translations/editor/sv.po", "editor/translations/editor/ta.po", "editor/translations/editor/th.po", "editor/translations/editor/tok.po", "editor/translations/editor/tr.po", "editor/translations/editor/uk.po", "editor/translations/editor/vi.po", "editor/translations/editor/zh_Hans.po", "editor/translations/editor/zh_Hant.po"],
            )
        case 1:
            make_translations(
                ["editor/translations/property_translations.gen.h", "editor/translations/property_translations.gen.cpp"],
                ["editor/translations/properties/bg.po", "editor/translations/properties/ca.po", "editor/translations/properties/cs.po", "editor/translations/properties/de.po", "editor/translations/properties/es.po", "editor/translations/properties/et.po", "editor/translations/properties/fa.po", "editor/translations/properties/fr.po", "editor/translations/properties/ga.po", "editor/translations/properties/hi.po", "editor/translations/properties/id.po", "editor/translations/properties/it.po", "editor/translations/properties/ja.po", "editor/translations/properties/ka.po", "editor/translations/properties/ko.po", "editor/translations/properties/pl.po", "editor/translations/properties/pt.po", "editor/translations/properties/pt_BR.po", "editor/translations/properties/ru.po", "editor/translations/properties/sv.po", "editor/translations/properties/ta.po", "editor/translations/properties/tr.po", "editor/translations/properties/uk.po", "editor/translations/properties/vi.po", "editor/translations/properties/zh_Hans.po", "editor/translations/properties/zh_Hant.po"],
            )
        case _:
            pass
