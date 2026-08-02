#!/usr/bin/env python3

"""Functions used to generate source files during build time"""

import os.path
import utils

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


def include_file_in_rd_header(
    filename: str, header_data: RDHeaderStruct, depth: int
) -> RDHeaderStruct:
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
                    included_file = os.path.relpath(
                        os.path.dirname(filename) + "/" + includeline
                    )

                if (
                    included_file not in header_data.vertex_included_files
                    and header_data.reading == "vertex"
                ):
                    header_data.vertex_included_files += [included_file]
                    if (
                        include_file_in_rd_header(included_file, header_data, depth + 1)
                        is None
                    ):
                        print_error(
                            f'In file "{filename}": #include "{includeline}" could not be found!"'
                        )
                elif (
                    included_file not in header_data.fragment_included_files
                    and header_data.reading == "fragment"
                ):
                    header_data.fragment_included_files += [included_file]
                    if (
                        include_file_in_rd_header(included_file, header_data, depth + 1)
                        is None
                    ):
                        print_error(
                            f'In file "{filename}": #include "{includeline}" could not be found!"'
                        )
                elif (
                    included_file not in header_data.compute_included_files
                    and header_data.reading == "compute"
                ):
                    header_data.compute_included_files += [included_file]
                    if (
                        include_file_in_rd_header(included_file, header_data, depth + 1)
                        is None
                    ):
                        print_error(
                            f'In file "{filename}": #include "{includeline}" could not be found!"'
                        )
                elif (
                    included_file not in header_data.raygen_included_files
                    and header_data.reading == "raygen"
                ):
                    header_data.raygen_included_files += [included_file]
                    if (
                        include_file_in_rd_header(included_file, header_data, depth + 1)
                        is None
                    ):
                        print_error(
                            f'In file "{filename}": #include "{includeline}" could not be found!"'
                        )
                elif (
                    included_file not in header_data.any_hit_included_files
                    and header_data.reading == "any_hit"
                ):
                    header_data.any_hit_included_files += [included_file]
                    if (
                        include_file_in_rd_header(included_file, header_data, depth + 1)
                        is None
                    ):
                        print_error(
                            f'In file "{filename}": #include "{includeline}" could not be found!"'
                        )
                elif (
                    included_file not in header_data.closest_hit_included_files
                    and header_data.reading == "closest_hit"
                ):
                    header_data.closest_hit_included_files += [included_file]
                    if (
                        include_file_in_rd_header(included_file, header_data, depth + 1)
                        is None
                    ):
                        print_error(
                            f'In file "{filename}": #include "{includeline}" could not be found!"'
                        )
                elif (
                    included_file not in header_data.miss_included_files
                    and header_data.reading == "miss"
                ):
                    header_data.miss_included_files += [included_file]
                    if (
                        include_file_in_rd_header(included_file, header_data, depth + 1)
                        is None
                    ):
                        print_error(
                            f'In file "{filename}": #include "{includeline}" could not be found!"'
                        )
                elif (
                    included_file not in header_data.intersection_included_files
                    and header_data.reading == "intersection"
                ):
                    header_data.intersection_included_files += [included_file]
                    if (
                        include_file_in_rd_header(included_file, header_data, depth + 1)
                        is None
                    ):
                        print_error(
                            f'In file "{filename}": #include "{includeline}" could not be found!"'
                        )

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


def build_rd_header(filename: str, shader: str) -> None:
    include_file_in_rd_header(shader, header_data := RDHeaderStruct(), 0)
    class_name = (
        os.path.basename(shader)
        .replace(".glsl", "")
        .title()
        .replace("_", "")
        .replace(".", "")
        + "ShaderRD"
    )

    with generated_wrapper(filename) as file:
        file.write(f"""\
#include "servers/rendering/renderer_rd/shader_rd.h"

class {class_name} : public ShaderRD {{
public:
	{class_name}() {{
""")

        if (
            header_data.raygen_lines
            or header_data.any_hit_lines
            or header_data.closest_hit_lines
            or header_data.miss_lines
            or header_data.intersection_lines
        ):
            file.write(
                build_rd_header_lines_for_raytracing_stage(
                    header_data.raygen_lines, "raygen"
                )
            )
            file.write(
                build_rd_header_lines_for_raytracing_stage(
                    header_data.any_hit_lines, "any_hit"
                )
            )
            file.write(
                build_rd_header_lines_for_raytracing_stage(
                    header_data.closest_hit_lines, "closest_hit"
                )
            )
            file.write(
                build_rd_header_lines_for_raytracing_stage(
                    header_data.miss_lines, "miss"
                )
            )
            file.write(
                build_rd_header_lines_for_raytracing_stage(
                    header_data.intersection_lines, "intersection"
                )
            )
            file.write(f"""\
		setup_raytracing(_raygen_code, _any_hit_code, _closest_hit_code, _miss_code, _intersection_code, "{class_name}");
""")
        elif header_data.compute_lines:
            file.write(f"""\
		static const char *_vertex_code = nullptr;
		static const char *_fragment_code = nullptr;
		static const char _compute_code[] = {{
{to_raw_cstring(header_data.compute_lines)}
		}};
		setup(_vertex_code, _fragment_code, _compute_code, "{class_name}");
""")
        else:
            file.write(f"""\
		static const char _vertex_code[] = {{
{to_raw_cstring(header_data.vertex_lines)}
		}};
		static const char _fragment_code[] = {{
{to_raw_cstring(header_data.fragment_lines)}
		}};
		static const char *_compute_code = nullptr;
		setup(_vertex_code, _fragment_code, _compute_code, "{class_name}");
""")

        file.write("""\
	}
};
""")


def build_rd_headers(target, source):
    for src in source:
        build_rd_header(f"{src}.gen.h", str(src))


class RAWHeaderStruct:
    def __init__(self):
        self.code = ""


def include_file_in_raw_header(
    filename: str, header_data: RAWHeaderStruct, depth: int
) -> None:
    with open(filename, "r", encoding="utf-8") as fs:
        line = fs.readline()

        while line:
            while line.find("#include ") != -1:
                includeline = line.replace("#include ", "").strip()[1:-1]

                included_file = os.path.relpath(
                    os.path.dirname(filename) + "/" + includeline
                )
                include_file_in_raw_header(included_file, header_data, depth + 1)

                line = fs.readline()

            header_data.code += line
            line = fs.readline()


run = utils.get_run_arg()
match run:
    case 0:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/bokeh_dof.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/bokeh_dof.glsl"],
        )
    case 1:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/bokeh_dof_raster.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/bokeh_dof_raster.glsl"],
        )
    case 2:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/blur_raster.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/blur_raster.glsl"],
        )
    case 3:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/copy.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/copy.glsl"],
        )
    case 4:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/copy_to_fb.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/copy_to_fb.glsl"],
        )
    case 5:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/cube_to_dp.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/cube_to_dp.glsl"],
        )
    case 6:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/cube_to_octmap.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/cube_to_octmap.glsl"],
        )
    case 7:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/octmap_downsampler.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/octmap_downsampler.glsl"],
        )
    case 8:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/octmap_downsampler_raster.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/octmap_downsampler_raster.glsl"
            ],
        )
    case 9:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/octmap_filter.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/octmap_filter.glsl"],
        )
    case 10:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/octmap_filter_raster.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/octmap_filter_raster.glsl"],
        )
    case 11:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/octmap_roughness.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/octmap_roughness.glsl"],
        )
    case 12:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/octmap_roughness_raster.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/octmap_roughness_raster.glsl"
            ],
        )
    case 13:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/specular_merge.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/specular_merge.glsl"],
        )
    case 14:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/motion_vectors.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/motion_vectors.glsl"],
        )
    case 15:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/shadow_frustum.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/shadow_frustum.glsl"],
        )
    case 16:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/fsr_upscale.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/fsr_upscale.glsl"],
        )
    case 17:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/luminance_reduce.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/luminance_reduce.glsl"],
        )
    case 18:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/luminance_reduce_raster.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/luminance_reduce_raster.glsl"
            ],
        )
    case 19:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/resolve.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/resolve.glsl"],
        )
    case 20:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/resolve_raster.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/resolve_raster.glsl"],
        )
    case 21:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/smaa_blending.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/smaa_blending.glsl"],
        )
    case 22:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/smaa_edge_detection.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/smaa_edge_detection.glsl"],
        )
    case 23:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/smaa_weight_calculation.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/smaa_weight_calculation.glsl"
            ],
        )
    case 24:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/tonemap.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/tonemap.glsl"],
        )
    case 25:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/tonemap_mobile.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/tonemap_mobile.glsl"],
        )
    case 26:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/vrs.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/vrs.glsl"],
        )
    case 27:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/environment/gi.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/environment/gi.glsl"],
        )
    case 28:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/environment/sdfgi_debug.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/environment/sdfgi_debug.glsl"],
        )
    case 29:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/environment/sdfgi_debug_probes.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/environment/sdfgi_debug_probes.glsl"
            ],
        )
    case 30:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/environment/sdfgi_direct_light.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/environment/sdfgi_direct_light.glsl"
            ],
        )
    case 31:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/environment/sdfgi_integrate.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/environment/sdfgi_integrate.glsl"],
        )
    case 32:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/environment/sdfgi_preprocess.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/environment/sdfgi_preprocess.glsl"],
        )
    case 33:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/environment/voxel_gi.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/environment/voxel_gi.glsl"],
        )
    case 34:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/environment/voxel_gi_debug.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/environment/voxel_gi_debug.glsl"],
        )
    case 35:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/environment/sky.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/environment/sky.glsl"],
        )
    case 36:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/canvas_sdf.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/canvas_sdf.glsl"],
        )
    case 37:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/tex_blit.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/tex_blit.glsl"],
        )
    case 38:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/cluster_debug.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/cluster_debug.glsl"],
        )
    case 39:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/cluster_render.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/cluster_render.glsl"],
        )
    case 40:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/cluster_store.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/cluster_store.glsl"],
        )
    case 41:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/blit.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/blit.glsl"],
        )
    case 42:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/environment/volumetric_fog.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/environment/volumetric_fog.glsl"],
        )
    case 43:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/environment/volumetric_fog_process.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/environment/volumetric_fog_process.glsl"
            ],
        )
    case 44:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/canvas.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/canvas.glsl"],
        )
    case 45:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/canvas_occlusion.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/canvas_occlusion.glsl"],
        )
    case 46:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/skeleton.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/skeleton.glsl"],
        )
    case 47:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/particles.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/particles.glsl"],
        )
    case 48:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/particles_copy.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/particles_copy.glsl"],
        )
    case 49:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/forward_clustered/best_fit_normal.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/forward_clustered/best_fit_normal.glsl"
            ],
        )
    case 50:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/forward_clustered/integrate_dfg.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/forward_clustered/integrate_dfg.glsl"
            ],
        )
    case 51:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_accumulate_pass.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_accumulate_pass.glsl"
            ],
        )
    case 52:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_autogen_reactive_pass.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_autogen_reactive_pass.glsl"
            ],
        )
    case 53:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_compute_luminance_pyramid_pass.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_compute_luminance_pyramid_pass.glsl"
            ],
        )
    case 54:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_depth_clip_pass.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_depth_clip_pass.glsl"
            ],
        )
    case 55:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_lock_pass.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_lock_pass.glsl"],
        )
    case 56:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_rcas_pass.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_rcas_pass.glsl"],
        )
    case 57:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_reconstruct_previous_depth_pass.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_reconstruct_previous_depth_pass.glsl"
            ],
        )
    case 58:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_tcr_autogen_pass.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_tcr_autogen_pass.glsl"
            ],
        )
    case 59:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/motion_vectors_store.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/motion_vectors_store.glsl"],
        )
    case 60:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection.glsl"
            ],
        )
    case 61:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_downsample.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_downsample.glsl"
            ],
        )
    case 62:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_filter.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_filter.glsl"
            ],
        )
    case 63:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_hiz.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_hiz.glsl"
            ],
        )
    case 64:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_resolve.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_resolve.glsl"
            ],
        )
    case 65:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/ss_effects_downsample.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/ss_effects_downsample.glsl"
            ],
        )
    case 66:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/ssao.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/ssao.glsl"],
        )
    case 67:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/ssao_blur.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/ssao_blur.glsl"],
        )
    case 68:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/ssao_importance_map.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/ssao_importance_map.glsl"],
        )
    case 69:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/ssao_interleave.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/ssao_interleave.glsl"],
        )
    case 70:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/ssil.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/ssil.glsl"],
        )
    case 71:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/ssil_blur.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/ssil_blur.glsl"],
        )
    case 72:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/ssil_importance_map.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/ssil_importance_map.glsl"],
        )
    case 73:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/ssil_interleave.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/ssil_interleave.glsl"],
        )
    case 74:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/subsurface_scattering.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/effects/subsurface_scattering.glsl"
            ],
        )
    case 75:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/taa_resolve.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/taa_resolve.glsl"],
        )
    case 76:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/forward_clustered/scene_forward_clustered.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/forward_clustered/scene_forward_clustered.glsl"
            ],
        )
    case 77:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/forward_mobile/scene_forward_mobile.glsl.gen.h"
            ],
            [
                "servers/rendering/renderer_rd/shaders/forward_mobile/scene_forward_mobile.glsl"
            ],
        )
    case 78:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/effects/roughness_limiter.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/effects/roughness_limiter.glsl"],
        )
    case 79:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/effects/sort.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/effects/sort.glsl"],
        )
    case 80:
        build_rd_headers(
            [
                "servers/rendering/renderer_rd/shaders/environment/voxel_gi_sdf.glsl.gen.h"
            ],
            ["servers/rendering/renderer_rd/shaders/environment/voxel_gi_sdf.glsl"],
        )
    case 81:
        build_rd_headers(
            ["servers/rendering/renderer_rd/shaders/giprobe_write.glsl.gen.h"],
            ["servers/rendering/renderer_rd/shaders/giprobe_write.glsl"],
        )
    case _:
        print("No commands left to run.")
