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
    "USE_FUNC_REGULATION",
    "USE_GLOBAL_MEMORY",
    "MUT_RATE__INST_TAG_BF",
    "MUT_RATE__FUNC_TAG_BF",
    "NUM_ENV_STATES",
    "NUM_ENV_UPDATES",
    "CPU_CYCLES_PER_ENV_UPDATE"
]

possible_metrics = ["hamming", "streak", "symmetric wrap", "hash"]
possible_selectors = ["ranked"]
possible_regulators = ["add", "mult"]

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

def extract_runlog_FOT(fpath, max_update, resolution=1):
    content = None
    # Open run.log, grab contents.
    with open(fpath, "r") as fp:
        content = fp.read().strip()
    content = content.split("\n")
    # For each line in the log, collect printed updata/best scores
    values = []
    final_score = {"update":None, "score":None}
    for line in content:
        if line[:6] == "update":
            line = line.split(";")
            update = int(line[0].split(":")[-1].strip())
            score = float(line[1].split(":")[-1].strip())
            if update % resolution == 0:
                values.append({"update":update, "score":score})
            final_score["update"] = update
            final_score["score"] = score
    # If run stopped early (e.g., solution found, smear final score to end)
    if (values[-1]["update"] < max_update):
        cur_update = values[-1]["update"]
        cur_update += resolution
        while cur_update <= max_update:
            values.append({"update":cur_update, "score":final_score["score"]})
            cur_update += resolution
    return values

def main():
    parser = argparse.ArgumentParser(description="Data aggregation script.")
    parser.add_argument("--data", type=str, nargs="+", help="Where should we pull data (one or more locations)?")
    parser.add_argument("--dump", type=str, help="Where to dump this?", default=".")
    parser.add_argument("--out_fname", type=str, help="What should we call the output file?", default="fot.csv")
    parser.add_argument("--res", type=int, default=1, help="What resolution (in generations) should we pull fitness information from logs?")
    args = parser.parse_args()
    data_dirs = args.data
    dump_dir = args.dump
    dump_fname = args.out_fname
    fot_res = args.res
    # Are all data directories for real?
    if any([not os.path.exists(loc) for loc in data_dirs]):
        print("Unable to locate all data directories. Able to locate:", {loc: os.path.exists(loc) for loc in data_dirs})
        exit(-1)
    # Aggregate a list of all runs
    run_dirs = [os.path.join(data_dir, run_dir) for data_dir in data_dirs for run_dir in os.listdir(data_dir) if "__SEED_" in run_dir]
    # sort run directories by seed to make easier on the eyes
    run_dirs.sort(key=lambda x : int(x.split("_")[-1]))
    print(f"Found {len(run_dirs)} run directories.")
    header = ",".join([key for key in key_settings] + ["score", "update"])
    out_content = header + "\n"
    # For each run, extract fitness over time
    for run in run_dirs:
        print(f"Extracting from {run}")
        run_config_path = os.path.join(run, "output", "run_config.csv")
        if not os.path.exists(run_config_path):
            print(f"Failed to find run parameters ({run_config_path})")
            exit(-1)
        run_log_path = os.path.join(run, "run.log")
        if not os.path.exists(run_log_path):
            print(f"Failed to find run.log ({run_log_path})")
            exit(-1)
        # Extract run settings.
        run_settings = extract_settings(run_config_path)
        # Extract fitness over time information from run log
        fot = extract_runlog_FOT(run_log_path, max_update=int(run_settings["GENERATIONS"]), resolution=fot_res)
        # Dump fitness over time information into out_content.
        for entry in fot:
            out_content += ",".join([run_settings[key] for key in key_settings] + [str(entry["score"]), str(entry["update"])]) + "\n"
    # Write output to file.
    mkdir_p(dump_dir)
    with open(os.path.join(dump_dir, dump_fname), "w") as fp:
        fp.write(out_content)
    print(f"Done! Output written to {os.path.join(dump_dir, dump_fname)}")

if __name__ == "__main__":
    # extract_settings("./run.log")
    main()