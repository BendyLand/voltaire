#!/usr/bin/env python3

import os
from collections import OrderedDict
from io import TextIOWrapper

import methods


def _write_ltc_dds_bytes(file, source_path):
    with open(source_path, mode="rb") as binary:
        buffer = binary.read()[148:]  # skip .dds header
        file.write("0x{:02x}".format(buffer[0]))
        for byte in range(1, len(buffer)):
            file.write(",0x{:02x}".format(buffer[byte]))


def _write_controller_mappings(file, mapping_sources):
    platform_variables = {
        "Linux": "LINUXBSD",
        "Windows": "WINDOWS",
        "Mac OS X": "MACOS",
        "Android": "ANDROID",
        "iOS": "APPLE_EMBEDDED",
        "Web": "WEB",
    }
    platform_mappings = OrderedDict()
    for src_path in mapping_sources:
        with open(src_path, "r", encoding="utf-8") as f:
            mapping_file_lines = f.readlines()

        current_platform = None
        for line in mapping_file_lines:
            line = line.strip()
            if not line:
                continue
            if line[0] == "#":
                platform_or_header = line[1:].strip()
                if platform_or_header not in platform_variables:
                    continue
                current_platform = platform_or_header
                if current_platform not in platform_mappings:
                    platform_mappings[current_platform] = {}
            elif current_platform:
                guid = line.split(",")[0]
                if guid in platform_mappings[current_platform]:
                    file.write(
                        "// WARNING: DATABASE {} OVERWROTE PRIOR MAPPING: {} {}\n".format(
                            src_path,
                            current_platform,
                            platform_mappings[current_platform][guid],
                        )
                    )
                platform_mappings[current_platform][guid] = line

    file.write("const char *DefaultControllerMappings::mappings[] = {\n")
    for platform, mappings in platform_mappings.items():
        variable = platform_variables[platform]
        file.write(f"#ifdef {variable}_ENABLED\n")
        for mapping in mappings.values():
            file.write(f'\t"{mapping}",\n')
        file.write(f"#endif // {variable}_ENABLED\n")
    file.write("\tnullptr\n};\n")


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
                projects[content[0]] = projects.get(content[0], []) + [part]

            if not tag or not reader.current:
                if "License" in part and "Files" not in part:
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
    area_buf = methods.get_buffer("thirdparty/smaa/AreaTex.png")
    search_buf = methods.get_buffer("thirdparty/smaa/SearchTex.png")

    enc_hex = os.environ.get("SCRIPT_AES256_ENCRYPTION_KEY") or ("0" * 64)
    enc_buf = bytes.fromhex(enc_hex)
    if len(enc_buf) != 32:
        raise ValueError("Invalid SCRIPT_AES256_ENCRYPTION_KEY length")

    with open(str(target[0]), "w", encoding="utf-8", newline="\n") as file:
        file.write("/* THIS FILE IS GENERATED. EDITS WILL BE LOST. */\n\n")
        file.write("#ifndef CORE_LICENSE_GEN_H\n#define CORE_LICENSE_GEN_H\n\n")
        file.write("#include <cstdint>\n\n")
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

        file.write("inline constexpr ComponentCopyrightPart COPYRIGHT_PROJECT_PARTS[] = {\n")
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
            to_raw = ["" if line == "." else line for line in license[1:]]
            file.write(f"{methods.to_raw_cstring(to_raw)},\n\n")
        file.write("};\n\n")

        file.write("inline constexpr int LTC_LUT_DIMENSIONS = 64;\n")
        file.write("inline constexpr uint8_t LTC_LUT1[] = {")
        _write_ltc_dds_bytes(file, "servers/rendering/storage/ltc/ltc_lut1.dds")
        file.write("};\n\n")
        file.write("inline constexpr uint8_t LTC_LUT2[] = {")
        _write_ltc_dds_bytes(file, "servers/rendering/storage/ltc/ltc_lut2.dds")
        file.write("};\n\n")
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

#endif // CORE_LICENSE_GEN_H

#ifdef DEFINE_CORE_GENERATED_GLOBALS
#undef DEFINE_CORE_GENERATED_GLOBALS

extern const char *const VLTR_VERSION_HASH;
extern const unsigned long long VLTR_VERSION_TIMESTAMP;
const char *const VLTR_VERSION_HASH = "9c2e8974959335fb84c88681cc92f90475684411";
const unsigned long long VLTR_VERSION_TIMESTAMP = 1785356176;

uint8_t script_encryption_key[32] = {{
{methods.format_buffer(enc_buf, 1)}
}};

""")
        _write_controller_mappings(
            file,
            ["core/input/gamecontrollerdb.txt", "core/input/godotcontrollerdb.txt"],
        )
        file.write("\n#endif // DEFINE_CORE_GENERATED_GLOBALS\n")


if __name__ == "__main__":
    make_license_header(["core/license.gen.h"], ["COPYRIGHT.txt", "LICENSE.txt"])
