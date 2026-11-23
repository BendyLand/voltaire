#!/usr/bin/env python3

import methods


def modules_enabled_builder(target, source):
    modules = sorted(source[0].keys())
    with methods.generated_wrapper(str(target[0])) as file:
        for module in modules:
            upper = module.upper()
            file.write(f"#ifndef MODULE_{upper}_DISABLED\n")
            file.write(f"#define MODULE_{upper}_ENABLED\n")
            file.write("#endif\n")


if __name__ == "__main__":
    modules_enabled_builder(
        ["modules/modules_enabled.gen.h"],
        [methods.detect_modules("modules", recursive=False)],
    )
