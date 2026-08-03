import sys

import scripts.utils as utils

if __name__ == "__main__":
    if "full" in sys.argv:
        utils.run_full_build()
    elif "gen" in sys.argv:
        utils.generate_comp_file("compilation_commands")
    else:
        utils.run_incremental_build()
