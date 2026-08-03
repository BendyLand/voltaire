#!/usr/bin/env python3

import methods


def make_certs_header(target, source):
    buffer = methods.get_buffer(str(source[0]))
    decomp_size = len(buffer)
    buffer = methods.compress_buffer(buffer)

    with methods.generated_wrapper(str(target[0])) as file:
        # System certs path. Editor will use them if defined. (for package maintainers)
        file.write('#define _SYSTEM_CERTS_PATH "{}"\n'.format(source[2] or ""))
        if source[1]:
            # Defined here and not in env so changing it does not trigger a full rebuild.
            file.write(f"""\
#define BUILTIN_CERTS_ENABLED

inline constexpr int _certs_compressed_size = {len(buffer)};
inline constexpr int _certs_uncompressed_size = {decomp_size};
inline constexpr unsigned char _certs_compressed[] = {{
{methods.format_buffer(buffer, 1)}
}};
""")


make_certs_header(
    ["core/io/certs_compressed.gen.h"], ["thirdparty/certs/ca-bundle.crt", True, ""]
)
