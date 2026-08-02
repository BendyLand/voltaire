#!/usr/bin/env python3

from collections import OrderedDict
from io import TextIOWrapper

import methods


def version_hash_builder(target, source):
    with methods.generated_wrapper(str(target[0])) as file:
        file.write("""\
#include "core/version.h"

const char *const VLTR_VERSION_HASH = "{git_hash}";
const unsigned long long VLTR_VERSION_TIMESTAMP = {git_timestamp};
""".format(**source[0]))


version_hash_builder(
    ["core/version_hash.gen.cpp"],
    [
        {
            "git_hash": "9c2e8974959335fb84c88681cc92f90475684411",
            "git_timestamp": "1785356176",
        }
    ],
)
