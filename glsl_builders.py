"""Functions used to generate source files during build time"""

import os.path

from methods import generated_wrapper, print_error, to_raw_cstring


class RDHeaderStruct:
    def __init__(self):
        self.vertex_lines = []
        self.fragment_lines = []
        self.compute_lines = []
        self.raygen_lines = []
        self.any_hit_lines = []
        self.closest_hit_lines = []
        self.miss_lines = []
        self.intersection_lines = []

        self.vertex_included_files = []
        self.fragment_included_files = []
        self.compute_included_files = []
        self.raygen_included_files = []
        self.any_hit_included_files = []
        self.closest_hit_included_files = []
        self.miss_included_files = []
        self.intersection_included_files = []

        self.reading = ""
        self.line_offset = 0
        self.vertex_offset = 0
        self.fragment_offset = 0
        self.compute_offset = 0
        self.raygen_offset = 0
        self.any_hit_offset = 0
        self.closest_hit_offset = 0
        self.miss_offset = 0
        self.intersection_offset = 0


def include_file_in_rd_header(filename: str, header_data: RDHeaderStruct, depth: int) -> RDHeaderStruct:
    with open(filename, "r", encoding="utf-8") as fs:
        line = fs.readline()

        while line:
            index = line.find("//")
            if index != -1:
                line = line[:index]

            if line.find("#[vertex]") != -1:
                header_data.reading = "vertex"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.vertex_offset = header_data.line_offset
                continue

            if line.find("#[fragment]") != -1:
                header_data.reading = "fragment"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.fragment_offset = header_data.line_offset
                continue

            if line.find("#[compute]") != -1:
                header_data.reading = "compute"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.compute_offset = header_data.line_offset
                continue

            if line.find("#[raygen]") != -1:
                header_data.reading = "raygen"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.raygen_offset = header_data.line_offset
                continue

            if line.find("#[any_hit]") != -1:
                header_data.reading = "any_hit"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.any_hit_offset = header_data.line_offset
                continue

            if line.find("#[closest_hit]") != -1:
                header_data.reading = "closest_hit"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.closest_hit_offset = header_data.line_offset
                continue

            if line.find("#[miss]") != -1:
                header_data.reading = "miss"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.miss_offset = header_data.line_offset
                continue

            if line.find("#[intersection]") != -1:
                header_data.reading = "intersection"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.intersection_offset = header_data.line_offset
                continue

            while line.find("#include ") != -1:
                includeline = line.replace("#include ", "").strip()[1:-1]

                if includeline.startswith("thirdparty/"):
                    included_file = os.path.relpath(includeline)

                else:
                    included_file = os.path.relpath(os.path.dirname(filename) + "/" + includeline)

                if included_file not in header_data.vertex_included_files and header_data.reading == "vertex":
                    header_data.vertex_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif included_file not in header_data.fragment_included_files and header_data.reading == "fragment":
                    header_data.fragment_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif included_file not in header_data.compute_included_files and header_data.reading == "compute":
                    header_data.compute_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif included_file not in header_data.raygen_included_files and header_data.reading == "raygen":
                    header_data.raygen_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif included_file not in header_data.any_hit_included_files and header_data.reading == "any_hit":
                    header_data.any_hit_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif (
                    included_file not in header_data.closest_hit_included_files and header_data.reading == "closest_hit"
                ):
                    header_data.closest_hit_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif included_file not in header_data.miss_included_files and header_data.reading == "miss":
                    header_data.miss_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif (
                    included_file not in header_data.intersection_included_files
                    and header_data.reading == "intersection"
                ):
                    header_data.intersection_included_files += [included_file]
                    if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')

                line = fs.readline()

            line = line.replace("\r", "").replace("\n", "")

            if header_data.reading == "vertex":
                header_data.vertex_lines += [line]
            if header_data.reading == "fragment":
                header_data.fragment_lines += [line]
            if header_data.reading == "compute":
                header_data.compute_lines += [line]
            if header_data.reading == "raygen":
                header_data.raygen_lines += [line]
            if header_data.reading == "any_hit":
                header_data.any_hit_lines += [line]
            if header_data.reading == "closest_hit":
                header_data.closest_hit_lines += [line]
            if header_data.reading == "miss":
                header_data.miss_lines += [line]
            if header_data.reading == "intersection":
                header_data.intersection_lines += [line]

            line = fs.readline()
            header_data.line_offset += 1

    return header_data


def build_rd_header_lines_for_raytracing_stage(lines, stage: str):
    if lines:
        return f"""\
	static const char _{stage}_code[] = {{
{to_raw_cstring(lines)}
	}};
"""
    else:
        return f"""\
	static const char *_{stage}_code = nullptr;
"""


def build_rd_shader_body(shader: str) -> str:
    include_file_in_rd_header(shader, header_data := RDHeaderStruct(), 0)
    class_name = os.path.basename(shader).replace(".glsl", "").title().replace("_", "").replace(".", "") + "ShaderRD"

    out = [f"inline ShaderRD::StageData {class_name}() {{\n"]
    if (
        header_data.raygen_lines
        or header_data.any_hit_lines
        or header_data.closest_hit_lines
        or header_data.miss_lines
        or header_data.intersection_lines
    ):
        out.append(build_rd_header_lines_for_raytracing_stage(header_data.raygen_lines, "raygen"))
        out.append(build_rd_header_lines_for_raytracing_stage(header_data.any_hit_lines, "any_hit"))
        out.append(build_rd_header_lines_for_raytracing_stage(header_data.closest_hit_lines, "closest_hit"))
        out.append(build_rd_header_lines_for_raytracing_stage(header_data.miss_lines, "miss"))
        out.append(build_rd_header_lines_for_raytracing_stage(header_data.intersection_lines, "intersection"))
        out.append(f"""\
	ShaderRD::StageData d;
	d.raygen_code = _raygen_code;
	d.any_hit_code = _any_hit_code;
	d.closest_hit_code = _closest_hit_code;
	d.miss_code = _miss_code;
	d.intersection_code = _intersection_code;
	d.name = "{class_name}";
	d.is_raytracing = true;
	return d;
""")
    elif header_data.compute_lines:
        out.append(f"""\
	static const char _compute_code[] = {{
{to_raw_cstring(header_data.compute_lines)}
	}};
	ShaderRD::StageData d;
	d.compute_code = _compute_code;
	d.name = "{class_name}";
	return d;
""")
    else:
        out.append(f"""\
	static const char _vertex_code[] = {{
{to_raw_cstring(header_data.vertex_lines)}
	}};
	static const char _fragment_code[] = {{
{to_raw_cstring(header_data.fragment_lines)}
	}};
	ShaderRD::StageData d;
	d.vertex_code = _vertex_code;
	d.fragment_code = _fragment_code;
	d.name = "{class_name}";
	return d;
""")
    out.append("}\n")
    return "".join(out)


def build_rd_header(filename: str, shader: str) -> None:
    with generated_wrapper(filename) as file:
        file.write('#include "core/types.h"\n#include "servers/rendering/renderer_rd/shader_rd.h"\n\n')
        file.write(build_rd_shader_body(shader))


def build_rd_headers_bundle(
    filename: str,
    shaders: list[str],
    raw_shaders: list[str] | None = None,
    gles3_shaders: list[str] | None = None,
) -> None:
    os.makedirs(os.path.dirname(filename) or ".", exist_ok=True)
    with open(filename, "w", encoding="utf-8", newline="\n") as file:
        file.write("/* THIS FILE IS GENERATED. EDITS WILL BE LOST. */\n\n")
        file.write("#ifndef RD_SHADERS_GEN_H\n#define RD_SHADERS_GEN_H\n\n")
        file.write('#include "core/io/ip.h"\n\n')
        for idx, shader in enumerate(shaders):
            if idx:
                file.write("\n")
            file.write(build_rd_shader_body(shader))
        if raw_shaders:
            for shader in raw_shaders:
                file.write("\n")
                file.write(build_raw_shader_body(shader))
        file.write("\n#endif // RD_SHADERS_GEN_H\n")
        if gles3_shaders:
            file.write("\n#ifdef DEFINE_GLES3_SHADERS_GEN\n")
            file.write("#ifndef GLES3_SHADERS_GEN_EMITTED\n#define GLES3_SHADERS_GEN_EMITTED\n\n")
            for idx, shader in enumerate(gles3_shaders):
                if idx:
                    file.write("\n")
                _write_gles3_struct(file, shader)
            file.write("\n#endif // GLES3_SHADERS_GEN_EMITTED\n")
            file.write("#undef DEFINE_GLES3_SHADERS_GEN\n")
            file.write("#endif // DEFINE_GLES3_SHADERS_GEN\n")


def build_rd_headers(target, source, env):
    env.NoCache(target)
    for src in source:
        build_rd_header(f"{src}.gen.h", str(src))


class RAWHeaderStruct:
    def __init__(self):
        self.code = ""


def include_file_in_raw_header(filename: str, header_data: RAWHeaderStruct, depth: int) -> None:
    with open(filename, "r", encoding="utf-8") as fs:
        line = fs.readline()

        while line:
            while line.find("#include ") != -1:
                includeline = line.replace("#include ", "").strip()[1:-1]
                if includeline.startswith("thirdparty/"):
                    included_file = os.path.relpath(includeline)
                else:
                    included_file = os.path.relpath(os.path.dirname(filename) + "/" + includeline)
                include_file_in_raw_header(included_file, header_data, depth + 1)

                line = fs.readline()

            header_data.code += line
            line = fs.readline()


def build_raw_shader_body(shader: str) -> str:
    include_file_in_raw_header(shader, header_data := RAWHeaderStruct(), 0)
    return f"""\
inline constexpr const char {os.path.basename(shader).replace(".glsl", "_shader_glsl")}[] = {{
{to_raw_cstring(header_data.code)}
}};
"""


def build_raw_header(filename: str, shader: str) -> None:
    with generated_wrapper(filename) as file:
        file.write('#include "core/types.h"\n\n')
        file.write(build_raw_shader_body(shader))


def build_raw_headers_bundle(filename: str, shaders: list[str]) -> None:
    with generated_wrapper(filename) as file:
        file.write('#include "core/types.h"\n\n')
        for idx, shader in enumerate(shaders):
            if idx:
                file.write("\n")
            file.write(build_raw_shader_body(shader))


def build_raw_headers(target, source, env):
    env.NoCache(target)
    for src in source:
        build_raw_header(f"{src}.gen.h", str(src))


class GLES3HeaderStruct:
    def __init__(self):
        self.vertex_lines = []
        self.fragment_lines = []
        self.raygen_lines = []
        self.any_hit_lines = []
        self.closest_hit_lines = []
        self.miss_lines = []
        self.intersection_lines = []
        self.uniforms = []
        self.fbos = []
        self.texunits = []
        self.texunit_names = []
        self.ubos = []
        self.ubo_names = []
        self.feedbacks = []

        self.vertex_included_files = []
        self.fragment_included_files = []

        self.reading = ""
        self.line_offset = 0
        self.vertex_offset = 0
        self.fragment_offset = 0
        self.raygen_offset = 0
        self.any_hit_offset = 0
        self.closest_hit_offset = 0
        self.miss_offset = 0
        self.intersection_offset = 0
        self.variant_defines = []
        self.variant_names = []
        self.specialization_names = []
        self.specialization_values = []


def include_file_in_gles3_header(filename: str, header_data: GLES3HeaderStruct, depth: int):
    with open(filename, "r", encoding="utf-8") as fs:
        line = fs.readline()

        while line:
            if line.find("=") != -1 and header_data.reading == "":
                # Mode
                eqpos = line.find("=")
                defname = line[:eqpos].strip().upper()
                define = line[eqpos + 1 :].strip()
                header_data.variant_names.append(defname)
                header_data.variant_defines.append(define)
                line = fs.readline()
                header_data.line_offset += 1
                header_data.vertex_offset = header_data.line_offset
                continue

            if line.find("=") != -1 and header_data.reading == "specializations":
                # Specialization
                eqpos = line.find("=")
                specname = line[:eqpos].strip()
                specvalue = line[eqpos + 1 :]
                header_data.specialization_names.append(specname)
                header_data.specialization_values.append(specvalue)
                line = fs.readline()
                header_data.line_offset += 1
                header_data.vertex_offset = header_data.line_offset
                continue

            if line.find("#[modes]") != -1:
                # Nothing really, just skip
                line = fs.readline()
                header_data.line_offset += 1
                header_data.vertex_offset = header_data.line_offset
                continue

            if line.find("#[specializations]") != -1:
                header_data.reading = "specializations"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.vertex_offset = header_data.line_offset
                continue

            if line.find("#[vertex]") != -1:
                header_data.reading = "vertex"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.vertex_offset = header_data.line_offset
                continue

            if line.find("#[fragment]") != -1:
                header_data.reading = "fragment"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.fragment_offset = header_data.line_offset
                continue

            if line.find("#[raygen]") != -1:
                header_data.reading = "raygen"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.raygen_offset = header_data.line_offset
                continue

            if line.find("#[any_hit]") != -1:
                header_data.reading = "any_hit"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.any_hit_offset = header_data.line_offset
                continue

            if line.find("#[closest_hit]") != -1:
                header_data.reading = "closest_hit"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.closest_hit_offset = header_data.line_offset
                continue

            if line.find("#[miss]") != -1:
                header_data.reading = "miss"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.miss_offset = header_data.line_offset
                continue

            if line.find("#[intersection]") != -1:
                header_data.reading = "intersection"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.intersection_offset = header_data.line_offset
                continue

            while line.find("#include ") != -1:
                includeline = line.replace("#include ", "").strip()[1:-1]

                included_file = os.path.relpath(os.path.dirname(filename) + "/" + includeline)
                if included_file not in header_data.vertex_included_files and header_data.reading == "vertex":
                    header_data.vertex_included_files += [included_file]
                    if include_file_in_gles3_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')
                elif included_file not in header_data.fragment_included_files and header_data.reading == "fragment":
                    header_data.fragment_included_files += [included_file]
                    if include_file_in_gles3_header(included_file, header_data, depth + 1) is None:
                        print_error(f'In file "{filename}": #include "{includeline}" could not be found!"')

                line = fs.readline()

            if line.find("uniform") != -1 and line.lower().find("texunit:") != -1:
                # texture unit
                texunitstr = line[line.find(":") + 1 :].strip()
                if texunitstr == "auto":
                    texunit = "-1"
                else:
                    texunit = str(int(texunitstr))
                uline = line[: line.lower().find("//")]
                uline = uline.replace("uniform", "")
                uline = uline.replace("highp", "")
                uline = uline.replace(";", "")
                lines = uline.split(",")
                for x in lines:
                    x = x.strip()
                    x = x[x.rfind(" ") + 1 :]
                    if x.find("[") != -1:
                        # unfiorm array
                        x = x[: x.find("[")]

                    if x not in header_data.texunit_names:
                        header_data.texunits += [(x, texunit)]
                        header_data.texunit_names += [x]

            elif line.find("uniform") != -1 and line.lower().find("ubo:") != -1:
                # uniform buffer object
                ubostr = line[line.find(":") + 1 :].strip()
                ubo = str(int(ubostr))
                uline = line[: line.lower().find("//")]
                uline = uline[uline.find("uniform") + len("uniform") :]
                uline = uline.replace("highp", "")
                uline = uline.replace(";", "")
                uline = uline.replace("{", "").strip()
                lines = uline.split(",")
                for x in lines:
                    x = x.strip()
                    x = x[x.rfind(" ") + 1 :]
                    if x.find("[") != -1:
                        # unfiorm array
                        x = x[: x.find("[")]

                    if x not in header_data.ubo_names:
                        header_data.ubos += [(x, ubo)]
                        header_data.ubo_names += [x]

            elif line.find("uniform") != -1 and line.find("{") == -1 and line.find(";") != -1:
                uline = line.replace("uniform", "")
                uline = uline.replace(";", "")
                lines = uline.split(",")
                for x in lines:
                    x = x.strip()
                    x = x[x.rfind(" ") + 1 :]
                    if x.find("[") != -1:
                        # unfiorm array
                        x = x[: x.find("[")]

                    if x not in header_data.uniforms:
                        header_data.uniforms += [x]

            if (line.strip().find("out ") == 0 or line.strip().find("flat ") == 0) and line.find("tfb:") != -1:
                uline = line.replace("flat ", "")
                uline = uline.replace("out ", "")
                uline = uline.replace("highp ", "")
                uline = uline.replace(";", "")
                uline = uline[uline.find(" ") :].strip()

                if uline.find("//") != -1:
                    name, bind = uline.split("//")
                    if bind.find("tfb:") != -1:
                        name = name.strip()
                        bind = bind.replace("tfb:", "").strip()
                        header_data.feedbacks += [(name, bind)]

            line = line.replace("\r", "")
            line = line.replace("\n", "")

            if header_data.reading == "vertex":
                header_data.vertex_lines += [line]
            if header_data.reading == "fragment":
                header_data.fragment_lines += [line]
            if header_data.reading == "raygen":
                header_data.raygen_lines += [line]
            if header_data.reading == "any_hit":
                header_data.any_hit_lines += [line]
            if header_data.reading == "closest_hit":
                header_data.closest_hit_lines += [line]
            if header_data.reading == "miss":
                header_data.miss_lines += [line]
            if header_data.reading == "intersection":
                header_data.intersection_lines += [line]

            line = fs.readline()
            header_data.line_offset += 1

    return header_data


from io import StringIO


def _write_gles3_struct(file, shader: str) -> None:
    include_file_in_gles3_header(shader, header_data := GLES3HeaderStruct(), 0)
    out_file_class = (
        os.path.basename(shader).replace(".glsl", "").title().replace("_", "").replace(".", "") + "ShaderGLES3"
    )

    if True:
        defspec = 0
        defvariant = ""

        file.write(f"""\
struct {out_file_class} {{
	using TextureUniformData = ShaderGLES3::TextureUniformData;

	ShaderGLES3 shader;

""")

        if header_data.uniforms:
            uniforms = ",\n\t\t".join(uniform.upper() for uniform in header_data.uniforms)
            file.write(f"""\
	enum Uniforms {{
		{uniforms},
	}};

""")

        if header_data.variant_names:
            variant_names = ",\n\t\t".join(name for name in header_data.variant_names)
        else:
            variant_names = "DEFAULT"
            defvariant = " = DEFAULT"
        file.write(f"""\
	enum ShaderVariant {{
		{variant_names},
	}};

""")

        if header_data.specialization_names:
            specialization_names = ",\n\t\t".join(
                f"{name.upper()} = {1 << index}" for index, name in enumerate(header_data.specialization_names)
            )
            file.write(f"""\
	enum Specializations {{
		{specialization_names},
	}};

""")
        for index, specialization_value in enumerate(header_data.specialization_values):
            if specialization_value.strip().upper() in ["TRUE", "1"]:
                defspec |= 1 << index

        file.write(f"""\
	_FORCE_INLINE_ bool version_bind_shader(RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		return shader._version_bind_shader(p_version, p_variant, p_specialization);
	}}

""")

        if header_data.uniforms:
            file.write(f"""\
	_FORCE_INLINE_ int version_get_uniform(Uniforms p_uniform, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		return shader._version_get_uniform(p_uniform, p_version, p_variant, p_specialization);
	}}

	/* clang-format off */
#define TRY_GET_UNIFORM(var_name) int var_name = version_get_uniform(p_uniform, p_version, p_variant, p_specialization); if (var_name < 0) return
	/* clang-format on */

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, float p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1f(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, double p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1f(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, uint8_t p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1ui(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, int8_t p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1i(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, uint16_t p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1ui(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, int16_t p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1i(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, uint32_t p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1ui(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, int32_t p_value, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform1i(uniform_location, p_value);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Color &p_color, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		GLfloat col[4] = {{ p_color.r, p_color.g, p_color.b, p_color.a }};
		glUniform4fv(uniform_location, 1, col);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Vector2 &p_vec2, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		GLfloat vec2[2] = {{ float(p_vec2.x), float(p_vec2.y) }};
		glUniform2fv(uniform_location, 1, vec2);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Size2i &p_vec2, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		GLint vec2[2] = {{ GLint(p_vec2.x), GLint(p_vec2.y) }};
		glUniform2iv(uniform_location, 1, vec2);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Vector3 &p_vec3, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		GLfloat vec3[3] = {{ float(p_vec3.x), float(p_vec3.y), float(p_vec3.z) }};
		glUniform3fv(uniform_location, 1, vec3);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Vector4 &p_vec4, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		GLfloat vec4[4] = {{ float(p_vec4.x), float(p_vec4.y), float(p_vec4.z), float(p_vec4.w) }};
		glUniform4fv(uniform_location, 1, vec4);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, float p_a, float p_b, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform2f(uniform_location, p_a, p_b);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, float p_a, float p_b, float p_c, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform3f(uniform_location, p_a, p_b, p_c);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, float p_a, float p_b, float p_c, float p_d, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		glUniform4f(uniform_location, p_a, p_b, p_c, p_d);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Transform3D &p_transform, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		const Transform3D &tr = p_transform;

		GLfloat matrix[16] = {{ /* build a 16x16 matrix */
			(GLfloat)tr.basis.rows[0][0],
			(GLfloat)tr.basis.rows[1][0],
			(GLfloat)tr.basis.rows[2][0],
			(GLfloat)0,
			(GLfloat)tr.basis.rows[0][1],
			(GLfloat)tr.basis.rows[1][1],
			(GLfloat)tr.basis.rows[2][1],
			(GLfloat)0,
			(GLfloat)tr.basis.rows[0][2],
			(GLfloat)tr.basis.rows[1][2],
			(GLfloat)tr.basis.rows[2][2],
			(GLfloat)0,
			(GLfloat)tr.origin.x,
			(GLfloat)tr.origin.y,
			(GLfloat)tr.origin.z,
			(GLfloat)1
		}};

		glUniformMatrix4fv(uniform_location, 1, false, matrix);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Transform2D &p_transform, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		const Transform2D &tr = p_transform;

		GLfloat matrix[16] = {{ /* build a 16x16 matrix */
			(GLfloat)tr.columns[0][0],
			(GLfloat)tr.columns[0][1],
			(GLfloat)0,
			(GLfloat)0,
			(GLfloat)tr.columns[1][0],
			(GLfloat)tr.columns[1][1],
			(GLfloat)0,
			(GLfloat)0,
			(GLfloat)0,
			(GLfloat)0,
			(GLfloat)1,
			(GLfloat)0,
			(GLfloat)tr.columns[2][0],
			(GLfloat)tr.columns[2][1],
			(GLfloat)0,
			(GLfloat)1
		}};

		glUniformMatrix4fv(uniform_location, 1, false, matrix);
	}}

	_FORCE_INLINE_ void version_set_uniform(Uniforms p_uniform, const Projection &p_matrix, RID p_version, ShaderVariant p_variant{defvariant}, uint64_t p_specialization = {defspec}) {{
		TRY_GET_UNIFORM(uniform_location);
		GLfloat matrix[16];

		for (int i = 0; i < 4; i++) {{
			for (int j = 0; j < 4; j++) {{
				matrix[i * 4 + j] = p_matrix.columns[i][j];
			}}
		}}

		glUniformMatrix4fv(uniform_location, 1, false, matrix);
	}}

#undef TRY_GET_UNIFORM

""")

        file.write("""\
	RID version_create() {
		return shader.version_create();
	}

	void version_set_code(RID p_version, const HashMap<String, String> &p_code, const String &p_uniforms, const String &p_vertex_globals, const String &p_fragment_globals, const Vector<String> &p_custom_defines, const LocalVector<ShaderGLES3::TextureUniformData> &p_texture_uniforms, bool p_initialize = false) {
		shader.version_set_code(p_version, p_code, p_uniforms, p_vertex_globals, p_fragment_globals, p_custom_defines, p_texture_uniforms, p_initialize);
	}

	bool version_is_valid(RID p_version) {
		return shader.version_is_valid(p_version);
	}

	bool version_free(RID p_version) {
		return shader.version_free(p_version);
	}

	RenderingServerTypes::ShaderNativeSourceCode version_get_native_source_code(RID p_version) {
		return shader.version_get_native_source_code(p_version);
	}

	void initialize(const String &p_general_defines = "", int p_base_texture_index = 0) {
		_init();
		shader.initialize(p_general_defines, p_base_texture_index);
	}

	void _init() {
""")

        if header_data.uniforms:
            uniforms = ",\n\t\t\t".join(f'"{uniform}"' for uniform in header_data.uniforms)
            file.write(f"""\
		static const char *_uniform_strings[] = {{
			{uniforms}
		}};
""")
        else:
            file.write("""\
		static const char **_uniform_strings = nullptr;
""")

        if header_data.variant_defines:
            variant_count = len(header_data.variant_defines)
            variant_defines = ",\n\t\t\t".join(f'"{define}"' for define in header_data.variant_defines)
            file.write(f"""\
		static const char *_variant_defines[] = {{
			{variant_defines},
		}};
""")
        else:
            variant_count = 1
            file.write("""\
		static const char *_variant_defines[] = {" "};
""")

        if header_data.texunits:
            texunits = ",\n\t\t\t".join(f'{{ "{name}", {texunit} }}' for name, texunit in header_data.texunits)
            file.write(f"""\
		static ShaderGLES3::TexUnitPair _texunit_pairs[] = {{
			{texunits},
		}};
""")
        else:
            file.write("""\
		static ShaderGLES3::TexUnitPair *_texunit_pairs = nullptr;
""")

        if header_data.ubos:
            ubos = ",\n\t\t\t".join(f'{{ "{name}", {ubo} }}' for name, ubo in header_data.ubos)
            file.write(f"""\
		static ShaderGLES3::UBOPair _ubo_pairs[] = {{
			{ubos},
		}};
""")
        else:
            file.write("""\
		static ShaderGLES3::UBOPair *_ubo_pairs = nullptr;
""")

        if header_data.specialization_names:
            specializations = ",\n\t\t\t".join(
                f'{{ "{name}", {"true" if header_data.specialization_values[index].strip().upper() in ["TRUE", "1"] else "false"} }}'
                for index, name in enumerate(header_data.specialization_names)
            )
            file.write(f"""\
		static ShaderGLES3::Specialization _spec_pairs[] = {{
			{specializations},
		}};
""")
        else:
            file.write("""\
		static ShaderGLES3::Specialization *_spec_pairs = nullptr;
""")

        if header_data.feedbacks:
            feedbacks = ",\n\t\t\t".join(
                f'{{ "{name}", {0 if spec not in header_data.specialization_names else (1 << header_data.specialization_names.index(spec))} }}'
                for name, spec in header_data.feedbacks
            )
            file.write(f"""\
		static const ShaderGLES3::Feedback _feedbacks[] = {{
			{feedbacks},
		}};
""")
        else:
            file.write("""\
		static const ShaderGLES3::Feedback *_feedbacks = nullptr;
""")

        file.write(f"""\
		static const char _vertex_code[] = {{
{to_raw_cstring(header_data.vertex_lines)}
		}};

		static const char _fragment_code[] = {{
{to_raw_cstring(header_data.fragment_lines)}
		}};

		shader._setup(_vertex_code, _fragment_code, "{out_file_class}",
				{len(header_data.uniforms)}, _uniform_strings, {len(header_data.ubos)}, _ubo_pairs,
				{len(header_data.feedbacks)}, _feedbacks, {len(header_data.texunits)}, _texunit_pairs,
				{len(header_data.specialization_names)}, _spec_pairs, {variant_count}, _variant_defines);
	}}
}};
""")
