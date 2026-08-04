import sys

import scripts.utils as utils

if __name__ == "__main__":
    if "full" in sys.argv:
        utils.run_full_build()
        utils.link_object_files()
    elif "gen" in sys.argv:
        utils.generate_comp_file("compilation_commands", "bin/objects.rsp")
    elif "link" in sys.argv:
        utils.link_object_files()
    else:
        utils.run_incremental_build()
        utils.link_object_files()
