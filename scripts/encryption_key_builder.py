#!/usr/bin/env python3

from collections import OrderedDict
from io import TextIOWrapper

import methods


def encryption_key_builder(target, source):
    src = source[0] or "0" * 64
    try:
        buffer = bytes.fromhex(src)
        if len(buffer) != 32:
            raise ValueError
    except ValueError:
        methods.print_error(
            f'Invalid AES256 encryption key, not 64 hexadecimal characters: "{src}".\n'
            "Unset `SCRIPT_AES256_ENCRYPTION_KEY` in your environment "
            "or make sure that it contains exactly 64 hexadecimal characters."
        )
        raise

    with methods.generated_wrapper(str(target[0])) as file:
        file.write(f"""\
#include <cstdint>

uint8_t script_encryption_key[32] = {{
{methods.format_buffer(buffer, 1)}
}};""")


encryption_key_builder(["core/script_encryption_key.gen.cpp"], [None])
