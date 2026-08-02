#!/usr/bin/env python3

import os

import methods


def modules_tests_builder(target, source):
    headers = sorted(
        [
            os.path.relpath(src.path, methods.base_folder).replace("\\", "/")
            for src in source
        ]
    )
    with methods.generated_wrapper(str(target[0])) as file:
        file.write("// IWYU pragma: begin_keep.\n")
        for header in headers:
            file.write(f'#include "{header}"\n')
        file.write("// IWYU pragma: end_keep.\n")
