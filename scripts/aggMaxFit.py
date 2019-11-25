'''
Script: agg_min_correct_networks.py
For each run, grab smallest correct solution network. If run has none, report none.
'''

import argparse, os, copy, errno, csv, re


key_settings = [
    "SEED",
    "mbin_selector",
    "mbin_metric",
    "mbin_thresh",
    "NUM_SIGNAL_RESPONSES",
    "NUM_ENV_CYCLES",
    "USE_FUNC_REGULATION",
    "USE_GLOBAL_MEMORY",
    "MUT_RATE__INST_TAG_BF",
    "MUT_RATE__FUNC_TAG_BF"
]

possible_metrics = ["hamming", "streak", "integer", "hash"]
possible_selectors = ["ranked"]

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
def extract_settings(log_path):
    settings = {}
    if (not os.path.exists(log_path)):
        print(f"Failed to extract settings from {log_path}")
        return settings
    run_log_content = ""
    with open(log_path, "r") as fp:
        run_log_content = fp.read()
    # (1) Extract matchbin info (selector, metric, threshold)
    # NOTE - this disgusting mess will get cleaned up if I output a csv specifying these settings!
    raw_mbin_configs = ""
    pattern = "Configured MatchBin:"
    for line in run_log_content.split("\n"):
        if line[:len(pattern)] == pattern:
            raw_mbin_configs = line
            break
    settings["mbin_selector"] = [sel for sel in possible_selectors if sel in raw_mbin_configs.lower()][0]
    settings["mbin_metric"] = [metric for metric in possible_metrics if metric in raw_mbin_configs.lower()][0]
    settings["mbin_thresh"] = raw_mbin_configs.split("ThreshRatio: ")[-1].split(")")[0]

    raw_configs = run_log_content.split("==============================")[2].split("\n")
    for line in raw_configs:
        if len(line) <= 3: continue
        if line[:3] == "set":
            key,val = line.split(" ")[1:3]
            settings[key] = val
    return settings

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
    print(f"Found {len(run_dirs)} run directories.")
    header_set = set() # Use this to guarantee all organism file headers match.
    # For each run, aggregate max fitness organism information.
    max_fits = []
    for run in run_dirs:
        log_path = os.path.join(run, "run.log")
        if not os.path.exists(log_path):
            print(f"Failed to find run.log ({log_path})")
            exit(-1)
        max_fit_path = os.path.join(run, "output", "max_fit_org.csv")
        if not os.path.exists(max_fit_path):
            print(f"Failed to find max_fit_org.csv ({max_fit_path})")
            exit(-1)
        # Extract run settings.
        run_settings = extract_settings(log_path)
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
            if orgs[i][header_lu["score"]] > orgs[best_org_id][header_lu["score"]]:
                best_org_id = i
        # Guarantee header uniqueness
        header_set.add(",".join([key for key in key_settings] + header))
        if len(header_set) > 1:
            print(f"Header mismatch! ({max_fit_path})")
            exit(-1)
        # Build organism line.
        # - do some special processing on program entry
        orgs[best_org_id][header_lu["program"]] = f"\"{orgs[best_org_id][header_lu['program']]}\""
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