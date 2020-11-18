'''
Script: aggMaxFit.py
For each run, grab the maximum fitness organism at end of run.

'''

import argparse, os, copy, errno, csv, re, sys
import json

csv.field_size_limit(sys.maxsize)

run_dir_identifier = "SEED_" # all legit run directories will have this substring in their name

config_exclude = {
    "STOP_ON_SOLUTION",
    "TESTING_SET_FILE",
    "TRAINING_SET_FILE",
    "OUTPUT_DIR",
    "SUMMARY_RESOLUTION",
    "SNAPSHOT_RESOLUTION",
    "OUTPUT_PROGRAMS",
    "input_signals",
    "input_signal_tags"
}

field_exclude = {
    "scores_by_test",
    "test_ids",
    "test_pass_distribution",
    "test_eval_distribution",
    "program"
}

"""
This is functionally equivalent to the mkdir -p [fname] bash command
"""
def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

"""
Given the path to a run's config file, extract the run's settings.
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

def find_org_analysis_path(run_path, update=None):
    output_path = os.path.join(run_path, "output")
    # Find all org analysis files (e.g., analysis_org_0_update_1000.csv)
    analysis_files = [fname for fname in os.listdir(output_path) if "analysis_org_" in fname]
    if len(analysis_files) == 0: return None
    def max_key(s):
        u = int(s.split("_update_")[-1].split(".")[0])
        if update == None:
            return u
        else:
            return u if u <= update else -1
    return os.path.join(output_path, max(analysis_files, key=max_key))

def find_trace_dir(run_path, update):
    output_path = os.path.join(run_path, "output")
    trace_dirs = [fname for fname in os.listdir(output_path) if "trace-" in fname]
    def max_key(s):
        u = int(s.split("-")[-1])
        if update == None:
            return u
        else:
            return u if u <= update else -1
    return os.path.join(output_path, max(trace_dirs, key=max_key))

"""
Aggregate!
"""
def main():
    # Setup the commandline argument parser
    parser = argparse.ArgumentParser(description="Data aggregation script.")
    parser.add_argument("--data", type=str, nargs="+", help="Where should we pull data (one or more locations)?")
    parser.add_argument("--dump", type=str, help="Where to dump this?", default=".")
    parser.add_argument("--update", type=int, default=-1, help="What is the maximum update we should pull organisms from?")
    parser.add_argument("--out_fname", type=str, help="What should we call the output file?", default="max_fit_orgs.csv")

    # Extract arguments from commandline
    args = parser.parse_args()
    data_dirs = args.data
    dump_dir = args.dump
    dump_fname = args.out_fname
    update = args.update

    # Are all data directories for real?
    if any([not os.path.exists(loc) for loc in data_dirs]):
        print("Unable to locate all data directories. Able to locate:", {loc: os.path.exists(loc) for loc in data_dirs})
        exit(-1)

    mkdir_p(dump_dir)

    # Aggregate a list of all runs
    run_dirs = [os.path.join(data_dir, run_dir) for data_dir in data_dirs for run_dir in os.listdir(data_dir) if run_dir_identifier in run_dir]

    # sort run directories by seed to make easier on the eyes
    run_dirs.sort(key=lambda x : int(x.split("_")[-1]))
    print(f"Found {len(run_dirs)} run directories.")

    analysis_header_set = set() # Use this to guarantee all organism file headers match.

    # --bookmark--

    # For each run, aggregate max fitness organism information.
    analysis_org_infos = []

    extra_fields = [
        "relies_on_regulation",
        "relies_on_global_memory",
        "relies_on_both",
        "relies_on_up_reg",
        "relies_on_down_reg",
        "ko_regulation_delta",
        "ko_global_memory_delta",
        "ko_all_delta",
        "ko_up_reg_delta",
        "ko_down_reg_delta",
        "update",
        "input_notation"
    ]

    for run in run_dirs:
        print(f"Extracting information from {run}")
        run_config_path = os.path.join(run, "output", "run_config.csv")
        org_analysis_path = find_org_analysis_path(run, update if update >= 0 else None)

        # does the run config file exist?
        if not os.path.exists(run_config_path):
            print(f"Failed to find run parameters ({run_config_path})")
            exit(-1)


        if not org_analysis_path:
            print(" >> no analysis files")
            continue

        # find organism analysis file
        if not os.path.exists(org_analysis_path):
            print(f"Failed to find run parameters ({org_analysis_path})")
            exit(-1)

        org_update = org_analysis_path.split(".")[0].split("_")[-1]

        # extract run settings
        run_settings = extract_settings(run_config_path)
        input_notation = run_settings["TRAINING_SET_FILE"].split("/")[-1].split(".")[0].replace("training_set", "")

        # ========= extract analysis file info =========
        content = None
        with open(org_analysis_path, "r") as fp:
            content = fp.read().strip().split("\n")
        analysis_header = content[0].split(",")
        analysis_header_lu = {analysis_header[i].strip():i for i in range(0, len(analysis_header))}
        content = content[1:]
        orgs = [l for l in csv.reader(content, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True)]
        org = orgs[-1] # each analysis file only has one line

        # -- collect extra fields --
        base_score = float(org[analysis_header_lu["aggregate_score"]])
        ko_reg_score = float(org[analysis_header_lu["ko_regulation_aggregate_score"]])
        ko_gmem_score = float(org[analysis_header_lu["ko_global_memory_aggregate_score"]])
        ko_all_score = float(org[analysis_header_lu["ko_all_aggregate_score"]])
        ko_up_reg_score = float(org[analysis_header_lu["ko_up_reg_aggregate_score"]])
        ko_down_reg_score = float(org[analysis_header_lu["ko_down_reg_aggregate_score"]])

        use_regulation = int(ko_reg_score < base_score)
        use_global_memory = int(ko_gmem_score < base_score)
        use_both = int(use_regulation and use_global_memory)

        relies_on_up_reg = int(ko_up_reg_score < base_score)
        relies_on_down_reg = int(ko_down_reg_score < base_score)

        ko_reg_delta = base_score - ko_reg_score
        ko_global_mem_delta = base_score - ko_gmem_score
        ko_all_delta = base_score - ko_all_score
        ko_up_reg_delta = base_score - ko_up_reg_score
        ko_down_reg_delta = base_score - ko_down_reg_score

        org_info = {
            "relies_on_regulation": use_regulation,
            "relies_on_global_memory": use_global_memory,
            "relies_on_both": use_both,
            "relies_on_up_reg": relies_on_up_reg,
            "relies_on_down_reg": relies_on_down_reg,
            "ko_regulation_delta": ko_reg_delta,
            "ko_global_memory_delta": ko_global_mem_delta,
            "ko_all_delta": ko_all_delta,
            "ko_up_reg_delta": ko_up_reg_delta,
            "ko_down_reg_delta": ko_down_reg_delta,
            "update": org_update,
            "input_notation": input_notation
        }

        # config_exclude field_exclude
        field_set = set(extra_fields)
        analysis_fields = [field for field in analysis_header if (field not in field_exclude) and (field not in field_set)]
        field_set.union(set(analysis_fields))
        config_fields = [field for field in run_settings if (field not in config_exclude) and (field not in field_set)]
        config_fields.sort()
        field_set.union(set(config_fields))

        fields = analysis_fields + extra_fields + config_fields
        analysis_header_set.add(",".join(fields))
        if len(analysis_header_set) > 1:
            print(f"Header mismatch! ({run})")
            exit(-1)

        # Add fields to org info
        for field in analysis_fields:
            org_info[field] = org[analysis_header_lu[field]]
        for field in config_fields:
            org_info[field] = run_settings[field]

        analysis_org_infos.append(",".join([str(org_info[field]) for field in fields]))

    # Output analysis org infos
    out_content = list(analysis_header_set)[0] + "\n"
    out_content += "\n".join(analysis_org_infos)
    with open(os.path.join(dump_dir, dump_fname), "w") as fp:
        fp.write(out_content)
    print(f" Done! Output written to {os.path.join(dump_dir, dump_fname)}")

if __name__ == "__main__":
    main()