#!/usr/bin/env python3

import os
import os.path
import shutil
import subprocess
import tempfile
import uuid

import methods
import utils


def make_translations(target, source):
    target_h, target_cpp = str(target[0]), str(target[1])

    category = os.path.basename(target_h).split("_")[0]
    sorted_paths = sorted(
        [os.path.abspath(src) for src in source],
        key=lambda path: os.path.splitext(os.path.basename(path))[0],
    )

    xl_names = []
    msgfmt = shutil.which("msgfmt")
    if not msgfmt:
        methods.print_warning("msgfmt not found, using .po files instead of .mo")

    with methods.generated_wrapper(target_cpp) as file:
        for path in sorted_paths:
            name = os.path.splitext(os.path.basename(path))[0]
            # msgfmt erases non-translated messages, so avoid using it if exporting the POT.
            if msgfmt and name != category:
                mo_path = os.path.join(tempfile.gettempdir(), uuid.uuid4().hex + ".mo")
                cmd = f'{msgfmt} "{path}" --no-hash -o "{mo_path}"'
                try:
                    subprocess.Popen(
                        cmd, shell=True, stderr=subprocess.PIPE
                    ).communicate()
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
                        # Do not fail the entire build if it cannot delete a temporary file.
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

            file.write(f"""\
inline constexpr const unsigned char _{category}_translation_{name}_compressed[] = {{
{methods.format_buffer(buffer, 1)}
}};

""")

            xl_names.append([name, len(buffer), decomp_size])

        file.write(f"""\
#include "{target_h}"

const EditorTranslationList _{category}_translations[] = {{
""")

        for x in xl_names:
            file.write(
                f'\t{{ "{x[0]}", {x[1]}, {x[2]}, _{category}_translation_{x[0]}_compressed }},\n'
            )

        file.write("""\
    { nullptr, 0, 0, nullptr },
};
""")

    with methods.generated_wrapper(target_h) as file:
        file.write(f"""\

#ifndef EDITOR_TRANSLATION_LIST
#define EDITOR_TRANSLATION_LIST

struct EditorTranslationList {{
    const char* lang;
    int comp_size;
    int uncomp_size;
    const unsigned char* data;
}};

#endif // EDITOR_TRANSLATION_LIST

extern const EditorTranslationList _{category}_translations[];
""")


run = utils.get_run_arg()
match run:
    case 0:
        make_translations(
            ["editor/translations/editor_translations.gen.h", "editor/translations/editor_translations.gen.cpp"],
            ["editor/translations/editor/ar.po", "editor/translations/editor/bg.po", "editor/translations/editor/bn.po", "editor/translations/editor/ca.po", "editor/translations/editor/cs.po", "editor/translations/editor/de.po", "editor/translations/editor/el.po", "editor/translations/editor/eo.po", "editor/translations/editor/es.po", "editor/translations/editor/es_AR.po", "editor/translations/editor/et.po", "editor/translations/editor/fa.po", "editor/translations/editor/fi.po", "editor/translations/editor/fr.po", "editor/translations/editor/ga.po", "editor/translations/editor/gl.po", "editor/translations/editor/he.po", "editor/translations/editor/hu.po", "editor/translations/editor/id.po", "editor/translations/editor/it.po", "editor/translations/editor/ja.po", "editor/translations/editor/ka.po", "editor/translations/editor/ko.po", "editor/translations/editor/nl.po", "editor/translations/editor/pl.po", "editor/translations/editor/pt.po", "editor/translations/editor/pt_BR.po", "editor/translations/editor/ro.po", "editor/translations/editor/ru.po", "editor/translations/editor/sk.po", "editor/translations/editor/sv.po", "editor/translations/editor/ta.po", "editor/translations/editor/th.po", "editor/translations/editor/tok.po", "editor/translations/editor/tr.po", "editor/translations/editor/uk.po", "editor/translations/editor/vi.po", "editor/translations/editor/zh_Hans.po", "editor/translations/editor/zh_Hant.po"]
        )
    case 1:
        make_translations(
            ["editor/translations/property_translations.gen.h", "editor/translations/property_translations.gen.cpp"],
            ["editor/translations/properties/bg.po", "editor/translations/properties/ca.po", "editor/translations/properties/cs.po", "editor/translations/properties/de.po", "editor/translations/properties/es.po", "editor/translations/properties/et.po", "editor/translations/properties/fa.po", "editor/translations/properties/fr.po", "editor/translations/properties/ga.po", "editor/translations/properties/hi.po", "editor/translations/properties/id.po", "editor/translations/properties/it.po", "editor/translations/properties/ja.po", "editor/translations/properties/ka.po", "editor/translations/properties/ko.po", "editor/translations/properties/pl.po", "editor/translations/properties/pt.po", "editor/translations/properties/pt_BR.po", "editor/translations/properties/ru.po", "editor/translations/properties/sv.po", "editor/translations/properties/ta.po", "editor/translations/properties/tr.po", "editor/translations/properties/uk.po", "editor/translations/properties/vi.po", "editor/translations/properties/zh_Hans.po", "editor/translations/properties/zh_Hant.po"]
        )
    case 2:
        make_translations(
            ["editor/translations/doc_translations.gen.h", "editor/translations/doc_translations.gen.cpp"],
            ["doc/translations/ca.po", "doc/translations/es.po", "doc/translations/fr.po", "doc/translations/ga.po", "doc/translations/it.po", "doc/translations/ko.po", "doc/translations/ru.po", "doc/translations/ta.po", "doc/translations/uk.po", "doc/translations/zh_Hans.po", "doc/translations/zh_Hant.po"]
        )
    case 3:
        make_translations(
            ["editor/translations/extractable_translations.gen.h", "editor/translations/extractable_translations.gen.cpp"],
            ["editor/translations/extractable/ar.po", "editor/translations/extractable/bg.po", "editor/translations/extractable/ca.po", "editor/translations/extractable/cs.po", "editor/translations/extractable/de.po", "editor/translations/extractable/el.po", "editor/translations/extractable/es.po", "editor/translations/extractable/es_AR.po", "editor/translations/extractable/et.po", "editor/translations/extractable/extractable.pot", "editor/translations/extractable/fa.po", "editor/translations/extractable/fi.po", "editor/translations/extractable/fr.po", "editor/translations/extractable/he.po", "editor/translations/extractable/id.po", "editor/translations/extractable/it.po", "editor/translations/extractable/ja.po", "editor/translations/extractable/ko.po", "editor/translations/extractable/lv.po", "editor/translations/extractable/nl.po", "editor/translations/extractable/pl.po", "editor/translations/extractable/pt.po", "editor/translations/extractable/pt_BR.po", "editor/translations/extractable/ru.po", "editor/translations/extractable/sk.po", "editor/translations/extractable/sv.po", "editor/translations/extractable/th.po", "editor/translations/extractable/tr.po", "editor/translations/extractable/uk.po", "editor/translations/extractable/vi.po", "editor/translations/extractable/zh_Hans.po", "editor/translations/extractable/zh_Hant.po"]
        )
    case _:
        print("No commands left to run.")
