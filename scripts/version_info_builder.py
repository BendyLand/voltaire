#!/usr/bin/env python3

from collections import OrderedDict
from io import TextIOWrapper

import methods


def version_info_builder(target, source):
    with methods.generated_wrapper(str(target[0])) as file:
        file.write("""\
#define VLTR_VERSION_SHORT_NAME "{short_name}"
#define VLTR_VERSION_NAME "{name}"
#define VLTR_VERSION_MAJOR {major}
#define VLTR_VERSION_MINOR {minor}
#define VLTR_VERSION_PATCH {patch}
#define VLTR_VERSION_STATUS "{status}"
#define VLTR_VERSION_BUILD "{build}"
#define VLTR_VERSION_MODULE_CONFIG "{module_config}"
#define VLTR_VERSION_WEBSITE "{website}"
#define VLTR_VERSION_DOCS_BRANCH "{docs_branch}"
#define VLTR_VERSION_DOCS_URL "https://docs.godotengine.org/en/" VLTR_VERSION_DOCS_BRANCH
""".format(**source[0]))


version_info_builder(
    ["core/version_generated.gen.h"],
    [
        {
            "short_name": "voltaire",
            "name": "Voltaire Engine",
            "major": 0,
            "minor": 0,
            "patch": 1,
            "status": "dev",
            "build": "custom_build",
            "module_config": "",
            "website": "blandlogic.com",
            "docs_branch": "latest",
        }
    ],
)
