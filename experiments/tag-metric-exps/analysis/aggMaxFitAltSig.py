'''
Script: aggMaxFit.py
For each run, grab the maximum fitness organism at end of run.
'''

import argparse, os, copy, errno, csv, re, sys

csv.field_size_limit(sys.maxsize)

key_settings = [
    "SEED",
    "matchbin_metric",
    "matchbin_thresh",
    "matchbin_regulator",
    "TAG_LEN",
    "NUM_SIGNAL_RESPONSES",
    "NUM_ENV_CYCLES",
    "USE_FUNC_REGULATION",
    "USE_GLOBAL_MEMORY",
    "MUT_RATE__INST_TAG_BF",
    "MUT_RATE__FUNC_TAG_BF",
    "CPU_TIME_PER_ENV_CYCLE"
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

def main():
    parser = argparse.ArgumentParser(description="Data aggregation script.")
    parser.add_argument("--data", type=str, nargs="+", help="Where should we pull data (one or more locations)?")
    parser.add_argument("--dump", type=str, help="Where to dump this?", default=".")
    parser.add_argument("--out_fname", type=str, help="What should we call the output file?", default="max_fit_orgs.csv")
    args = parser.parse_args()
    data_dirs = args.data
    dump_dir = args.dump
    dump_fname = args.out_fname
    # Are all data directories for real?
    if any([not os.path.exists(loc) for loc in data_dirs]):
        print("Unable to locate all data directories. Able to locate:", {loc: os.path.exists(loc) for loc in data_dirs})
        exit(-1)
    # Aggregate a list of all runs
    run_dirs = [os.path.join(data_dir, run_dir) for data_dir in data_dirs for run_dir in os.listdir(data_dir) if "__SEED_" in run_dir]
    # sort run directories by seed to make easier on the eyes
    run_dirs.sort(key=lambda x : int(x.split("_")[-1]))
    print(f"Found {len(run_dirs)} run directories.")
    header_set = set() # Use this to guarantee all organism file headers match.
    # For each run, aggregate max fitness organism information.
    max_fits = []
    for run in run_dirs:
        print(f"Extracting from {run}")
        run_config_path = os.path.join(run, "output", "run_config.csv")
        if not os.path.exists(run_config_path):
            print(f"Failed to find run parameters ({run_config_path})")
            exit(-1)
        max_fit_path = os.path.join(run, "output", "max_fit_org.csv")
        if not os.path.exists(max_fit_path):
            print(f"Failed to find max_fit_org.csv ({max_fit_path})")
            exit(-1)
        # Extract run settings.
        run_settings = extract_settings(run_config_path)
        # Find highest fitness organism for this run.
        content = None
        with open(max_fit_path, "r") as fp:
            content = fp.read().strip().split("\n")
        header = content[0].split(",")
        header_lu = {header[i].strip():i for i in range(0, len(header))}
        content = content[1:]
        orgs = [l for l in csv.reader(content, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True)]
        if len(orgs) < 1:
            print(f"Where 'dem fit organisms at? ({max_fit_path})")
            exit(-1)
        # Find the best organism
        best_org_id = 0
        for i in range(len(orgs)):
            if float(orgs[i][header_lu["score"]]) > float(orgs[best_org_id][header_lu["score"]]):
                best_org_id = i
        # Guarantee header uniqueness
        header_set.add(",".join([key for key in key_settings] + header))
        if len(header_set) > 1:
            print(f"Header mismatch! ({max_fit_path})")
            exit(-1)
        # Build organism line.
        # - do some special processing on program entry
        # orgs[best_org_id][header_lu["program"]] = f"\"{orgs[best_org_id][header_lu['program']]}\""
        orgs[best_org_id][header_lu["program"]] = "EXCLUDED"
        max_fits.append([run_settings[key] for key in key_settings] + orgs[best_org_id])
    # Output header + max_fit orgs
    out_content = list(header_set)[0] + "\n" # Should be guaranteed to be length 1!
    out_content += "\n".join([",".join(line) for line in max_fits])
    mkdir_p(dump_dir)
    with open(os.path.join(dump_dir, dump_fname), "w") as fp:
        fp.write(out_content)
    print(f"Done! Output written to {os.path.join(dump_dir, dump_fname)}")

if __name__ == "__main__":
    # extract_settings("./run.log")
    main()