import sys

import scripts.adv_utils as au
# import scripts.utils as utils

if __name__ == "__main__":
    if "full" in sys.argv:
        au.run_full_build()
        au.link_object_files()
    elif "gen" in sys.argv:
        au.generate_comp_file("compilation_commands", "bin/objects.rsp")
    elif "link" in sys.argv:
        au.link_object_files()
    else:
        au.run_incremental_build()
        au.link_object_files()
