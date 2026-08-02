#!/usr/bin/env python3

import methods


def register_platform_apis_builder(target, source):
    platforms = source[0]
    api_inc = "\n".join([f'#include "{p}/api/api.h"' for p in platforms])
    api_reg = "\n\t".join([f"register_{p}_api();" for p in platforms])
    api_unreg = "\n\t".join([f"unregister_{p}_api();" for p in platforms])
    with methods.generated_wrapper(str(target[0])) as file:
        file.write(
            f"""\
#include "register_platform_apis.h"

{api_inc}

void register_platform_apis() {{
	{api_reg}
}}

void unregister_platform_apis() {{
	{api_unreg}
}}
"""
        )

register_platform_apis_builder(["platform/register_platform_apis.gen.cpp"], [['android', 'ios', 'visionos', 'web']])
