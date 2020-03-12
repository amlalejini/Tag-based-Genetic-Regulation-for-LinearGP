'''
For each run, grab the maximum fitness organism over time.
'''

import argparse, os, copy, errno, csv, re, sys

csv.field_size_limit(sys.maxsize)

key_settings = [
    "SEED",
    "matchbin_metric",
    "matchbin_thresh",
    "matchbin_regulator",
    "TAG_LEN",
    "NUM_ENV_STATES",
    "NUM_ENV_UPDATES",
    "USE_FUNC_REGULATION",
    "USE_GLOBAL_MEMORY",
    "MUT_RATE__INST_TAG_BF",
    "MUT_RATE__FUNC_TAG_BF",
    "CPU_CYCLES_PER_ENV_UPDATE",
    "MAX_FUNC_CNT",
    "MAX_FUNC_INST_CNT",
    "MAX_ACTIVE_THREAD_CNT",
    "MAX_THREAD_CAPACITY",
    "MUT_RATE__FUNC_DUP"
]
# fitn



def mkdir_p(path):
    """
    This is functionally equivalent to the mkdir -p [fname] bash command
    """
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

"""
Given the path to a run.log, extract the run's settings.
"""
def extract_settings(run_config_path):
    content = None
    with open(run_config_path, "r") as fp:
        content = fp.read().strip().split("\n")
    header = content[0].split(",")
    header_lu = {header[i].strip():i for i in range(0, len(header))}
    content = content[1:]
    configs = [l for l in csv.reader(content, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True)]
    return {param[header_lu["parameter"]]:param[header_lu["value"]] for param in configs}

def main():
    parser = argparse.ArgumentParser(description="Data aggregation script.")
    parser.add_argument("--data", type=str, nargs="+", help="Where should we pull data (one or more locations)?")
    parser.add_argument("--dump", type=str, help="Where to dump this?", default=".")
    parser.add_argument("--out_fname", type=str, help="What should we call the output file?", default="fot.csv")
    parser.add_argument("--res", type=int, help="What resolution of fitness measures?", default=10)
    args = parser.parse_args()
    data_dirs = args.data
    dump_dir = args.dump
    dump_fname = args.out_fname
    resolution = args.res

    # Are all data directories for real?
    if any([not os.path.exists(loc) for loc in data_dirs]):
        print("Unable to locate all data directories. Able to locate:", {loc: os.path.exists(loc) for loc in data_dirs})
        exit(-1)

    # Aggregate a list of all runs
    run_dirs = [os.path.join(data_dir, run_dir) for data_dir in data_dirs for run_dir in os.listdir(data_dir) if "__SEED_" in run_dir]
    # sort run directories by seed to make easier on the eyes
    run_dirs.sort(key=lambda x : int(x.split("_")[-1]))
    print(f"Found {len(run_dirs)} run directories.")
    # header = ",".join([key for key in key_settings] + ["score", "update"])
    # out_content = header + "\n"

    out_fields = ["seed",
                    "tag_metric",
                    "tag_mut_rate",
                    "regulator",
                    "tag_width",
                    "NUM_ENV_STATES",
                    "NUM_ENV_UPDATES",
                    "score",
                    "update"]
    out_content = ",".join(out_fields) + "\n"

    # For each run, extract fitness over time
    for run in run_dirs:
        print(f"Extracting from {run}")
        run_config_path = os.path.join(run, "output", "run_config.csv")
        if not os.path.exists(run_config_path):
            print(f"Failed to find run parameters ({run_config_path})")
            exit(-1)
        fitness_path = os.path.join(run, "output", "fitness.csv")
        if not os.path.exists(fitness_path):
            print(f"Failed to find fitness data file ({fitness_path})")
            exit(-1)
        # Extract run settings.
        run_settings = extract_settings(run_config_path)
        # Extract fitness over time information from run log
        # fot = extract_runlog_FOT(run_log_path, max_update=int(run_settings["GENERATIONS"]), resolution=fot_res)
        # Extract fitness over time from fitness file
        content = None
        with open(fitness_path, "r") as fp:
            content = fp.read().strip().split("\n")
        header = content[0].split(",")
        header_lu = {header[i].strip():i for i in range(0, len(header))}
        content = content[1:]

        last_update = 0
        last_fitness = 0
        for line in content:
            line = line.split(",")
            info = {col:line[header_lu[col]] for col in header_lu}
            if run_settings["MUT_RATE__FUNC_TAG_BF"] != run_settings["MUT_RATE__INST_TAG_BF"]:
                print("!")
                exit(-1)
            out = {
                "seed": run_settings["SEED"],
                "tag_metric": run_settings["matchbin_metric"],
                "tag_mut_rate": run_settings["MUT_RATE__FUNC_TAG_BF"],
                "regulator": run_settings["matchbin_regulator"],
                "tag_width": run_settings["TAG_LEN"],
                "NUM_ENV_STATES": run_settings["NUM_ENV_STATES"],
                "NUM_ENV_UPDATES": run_settings["NUM_ENV_STATES"],
                "score": info["max_fitness"],
                "update": info["update"]
            }
            last_update = int(info["update"])
            last_fitness= float(info["max_fitness"])
            out_content += ",".join([out[field] for field in out_fields]) + "\n"
        max_update=int(run_settings["GENERATIONS"])
        while last_update < max_update:
            last_update += resolution
            out = {
                "seed": run_settings["SEED"],
                "tag_metric": run_settings["matchbin_metric"],
                "tag_mut_rate": run_settings["MUT_RATE__FUNC_TAG_BF"],
                "regulator": run_settings["matchbin_regulator"],
                "tag_width": run_settings["TAG_LEN"],
                "NUM_ENV_STATES": run_settings["NUM_ENV_STATES"],
                "NUM_ENV_UPDATES": run_settings["NUM_ENV_STATES"],
                "score": str(last_fitness),
                "update": str(last_update)
            }
            out_content += ",".join([out[field] for field in out_fields]) + "\n"

    # Write output to file.
    mkdir_p(dump_dir)
    with open(os.path.join(dump_dir, dump_fname), "w") as fp:
        fp.write(out_content)
    print(f"Done! Output written to {os.path.join(dump_dir, dump_fname)}")

if __name__ == "__main__":
    main()