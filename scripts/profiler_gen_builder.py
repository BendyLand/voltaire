#!/usr/bin/env python3

"""Functions used to generate profiling header files during build time"""

import os
import methods


def profiler_gen_builder(
    target,
    profiler=None,
    sample_callstack=False,
    track_memory=False,
    record_on_demand=False,
):
    # Fallback to environment variables if options aren't passed directly
    profiler = profiler or os.getenv("PROFILER", "none")
    sample_callstack = sample_callstack or (os.getenv("PROFILER_SAMPLE_CALLSTACK") == "1")
    track_memory = track_memory or (os.getenv("PROFILER_TRACK_MEMORY") == "1")
    record_on_demand = record_on_demand or (os.getenv("PROFILER_RECORD_ON_DEMAND") == "1")

    target_path = str(target[0]) if isinstance(target, (list, tuple)) else str(target)

    with methods.generated_wrapper(target_path) as file:
        if profiler == "tracy":
            file.write("#define VLTR_USE_TRACY\n")
            if sample_callstack:
                file.write("#define TRACY_CALLSTACK 62\n")
            if track_memory:
                file.write("#define VLTR_PROFILER_TRACK_MEMORY\n")
            if record_on_demand:
                file.write("#define TRACY_ON_DEMAND\n")

        elif profiler == "perfetto":
            file.write("#define VLTR_USE_PERFETTO\n")

        elif profiler == "instruments":
            file.write("#define VLTR_USE_INSTRUMENTS\n")
            if sample_callstack:
                file.write("#define INSTRUMENTS_SAMPLE_CALLSTACKS\n")


profiler_gen_builder(["core/profiling/profiling.gen.h"])
