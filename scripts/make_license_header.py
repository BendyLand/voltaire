#!/usr/bin/env python3

from collections import OrderedDict
from io import TextIOWrapper

import methods


def make_license_header(target, source):
    src_copyright = str(source[0])
    src_license = str(source[1])

    class LicenseReader:
        def __init__(self, license_file: TextIOWrapper):
            self._license_file = license_file
            self.line_num = 0
            self.current = self.next_line()

        def next_line(self):
            line = self._license_file.readline()
            self.line_num += 1
            while line.startswith("#"):
                line = self._license_file.readline()
                self.line_num += 1
            self.current = line
            return line

        def next_tag(self):
            if ":" not in self.current:
                return ("", [])
            tag, line = self.current.split(":", 1)
            lines = [line.strip()]
            while self.next_line() and self.current.startswith(" "):
                lines.append(self.current.strip())
            return (tag, lines)

    projects = OrderedDict()
    license_list = []

    with open(src_copyright, "r", encoding="utf-8") as copyright_file:
        reader = LicenseReader(copyright_file)
        part = {}
        while reader.current:
            tag, content = reader.next_tag()
            if tag in ("Files", "Copyright", "License"):
                part[tag] = content[:]
            elif tag == "Comment" and part:
                # attach non-empty part to named project
                projects[content[0]] = projects.get(content[0], []) + [part]

            if not tag or not reader.current:
                # end of a paragraph start a new part
                if "License" in part and "Files" not in part:
                    # no Files tag in this one, so assume standalone license
                    license_list.append(part["License"])
                part = {}
                reader.next_line()

    data_list = []
    for project in iter(projects.values()):
        for part in project:
            part["file_index"] = len(data_list)
            data_list += part["Files"]
            part["copyright_index"] = len(data_list)
            data_list += part["Copyright"]

    with open(src_license, "r", encoding="utf-8") as file:
        license_text = file.read()

    authors_sections = {
        "Project Founders": "AUTHORS_FOUNDERS",
        "Lead Developer": "AUTHORS_LEAD_DEVELOPERS",
        "Project Manager": "AUTHORS_PROJECT_MANAGERS",
        "Developers": "AUTHORS_DEVELOPERS",
    }
    donors_sections = {
        "Patrons": "DONORS_PATRONS",
        "Platinum sponsors": "DONORS_SPONSORS_PLATINUM",
        "Gold sponsors": "DONORS_SPONSORS_GOLD",
        "Silver sponsors": "DONORS_SPONSORS_SILVER",
        "Diamond members": "DONORS_MEMBERS_DIAMOND",
        "Titanium members": "DONORS_MEMBERS_TITANIUM",
        "Platinum members": "DONORS_MEMBERS_PLATINUM",
        "Gold members": "DONORS_MEMBERS_GOLD",
    }

    def write_md_sections(out_file, md_path, section_map):
        buf = methods.get_buffer(md_path)
        reading = False
        for ln in buf.decode().splitlines():
            if ln.startswith("    ") and reading:
                out_file.write(f'\t"{methods.to_escaped_cstring(ln).strip()}",\n')
            elif ln.startswith("## "):
                if reading:
                    out_file.write("\tnullptr,\n};\n\n")
                    reading = False
                sec = section_map.get(ln[3:].strip())
                if sec:
                    out_file.write(f"inline constexpr const char *{sec}[] = {{\n")
                    reading = True
        if reading:
            out_file.write("\tnullptr,\n};\n\n")

    certs_buf = methods.get_buffer("thirdparty/certs/ca-bundle.crt")
    certs_decomp_size = len(certs_buf)
    certs_comp = methods.compress_buffer(certs_buf)

    with methods.generated_wrapper(str(target[0])) as file:
        file.write('#define _SYSTEM_CERTS_PATH ""\n#define BUILTIN_CERTS_ENABLED\n\n')
        file.write(f"inline constexpr int _certs_compressed_size = {len(certs_comp)};\n")
        file.write(f"inline constexpr int _certs_uncompressed_size = {certs_decomp_size};\n")
        file.write(f"inline constexpr unsigned char _certs_compressed[] = {{\n{methods.format_buffer(certs_comp, 1)}\n}};\n\n")
        write_md_sections(file, "AUTHORS.md", authors_sections)
        write_md_sections(file, "DONORS.md", donors_sections)
        file.write(f"""\
inline constexpr const char *GODOT_LICENSE_TEXT = {{
{methods.to_raw_cstring(license_text)}
}};

struct ComponentCopyrightPart {{
	const char *license;
	const char *const *files;
	const char *const *copyright_statements;
	int file_count;
	int copyright_count;
}};

struct ComponentCopyright {{
	const char *name;
	const ComponentCopyrightPart *parts;
	int part_count;
}};

""")

        file.write("inline constexpr const char *COPYRIGHT_INFO_DATA[] = {\n")
        for line in data_list:
            file.write(f'\t"{methods.to_escaped_cstring(line)}",\n')
        file.write("};\n\n")

        file.write(
            "inline constexpr ComponentCopyrightPart COPYRIGHT_PROJECT_PARTS[] = {\n"
        )
        part_index = 0
        part_indexes = {}
        for project_name, project in iter(projects.items()):
            part_indexes[project_name] = part_index
            for part in project:
                file.write(
                    f'\t{{ "{methods.to_escaped_cstring(part["License"][0])}", '
                    + f"&COPYRIGHT_INFO_DATA[{part['file_index']}], "
                    + f"&COPYRIGHT_INFO_DATA[{part['copyright_index']}], "
                    + f"{len(part['Files'])}, {len(part['Copyright'])} }},\n"
                )
                part_index += 1
        file.write("};\n\n")

        file.write(f"inline constexpr int COPYRIGHT_INFO_COUNT = {len(projects)};\n")

        file.write("inline constexpr ComponentCopyright COPYRIGHT_INFO[] = {\n")
        for project_name, project in iter(projects.items()):
            file.write(
                f'\t{{ "{methods.to_escaped_cstring(project_name)}", '
                + f"&COPYRIGHT_PROJECT_PARTS[{part_indexes[project_name]}], "
                + f"{len(project)} }},\n"
            )
        file.write("};\n\n")

        file.write(f"inline constexpr int LICENSE_COUNT = {len(license_list)};\n")

        file.write("inline constexpr const char *LICENSE_NAMES[] = {\n")
        for license in license_list:
            file.write(f'\t"{methods.to_escaped_cstring(license[0])}",\n')
        file.write("};\n\n")

        file.write("inline constexpr const char *LICENSE_BODIES[] = {\n\n")
        for license in license_list:
            to_raw = []
            for line in license[1:]:
                if line == ".":
                    to_raw += [""]
                else:
                    to_raw += [line]
            file.write(f"{methods.to_raw_cstring(to_raw)},\n\n")
        file.write("};\n\n")


make_license_header(["core/license.gen.h"], ["COPYRIGHT.txt", "LICENSE.txt"])
