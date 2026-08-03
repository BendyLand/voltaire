#!/usr/bin/env python3

from collections import OrderedDict
from io import TextIOWrapper

import methods


# Generate disabled classes
def disabled_class_builder(target, source):
    with methods.generated_wrapper(str(target[0])) as file:
        for c in source[0]:
            if cs := c.strip():
                file.write(
                    f"class {cs}; template <> struct is_class_enabled<{cs}> : std::false_type {{}};\n"
                )


disabled_class_builder(["core/disabled_classes.gen.h"], [[]])
