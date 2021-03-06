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
    "USE_FUNC_REGULATION",
    "USE_GLOBAL_MEMORY",
    "MUT_RATE__INST_TAG_BF",
    "MUT_RATE__FUNC_TAG_BF",
    "DEME_WIDTH",
    "DEME_HEIGHT",
    "NUM_RESPONSE_TYPES",
    "CUSTOM_MAX_RESPONSE_CNT",
    "MAX_RESPONSE_CNT",
    "DEVELOPMENT_PHASE_CPU_TIME",
    "RESPONSE_PHASE_CPU_TIME",
    "PROPAGULE_SIZE",
    "PROPAGULE_LAYOUT",
    "EPIGENETIC_INHERITANCE",
    "MAX_ACTIVE_THREAD_CNT",
    "USE_RANDOM_CELL_SCHEDULING",
    "FIXED_REPRO_TAG",
    "ALLOW_MESSAGING",
    "SCORE_RESPONSE_MODE"
]

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

def extract_org_analyses():
    pass

def extract_org_traces():
    pass

def find_org_analysis_path(run_path, update):
    output_path = os.path.join(run_path, "output")
    # Find all org analysis files (analysis_org_0_update_1000.csv)
    analysis_files = [fname for fname in os.listdir(output_path) if "analysis_org_" in fname]
    def max_key(s):
        u = int(s.split("_update_")[-1].split(".")[0])
        if update == None:
            return u
        else:
            return u if u <= update else -1
    return os.path.join(output_path, max(analysis_files, key=max_key))

def find_trace_path(run_path, update):
    output_path = os.path.join(run_path, "output")
    trace_files = [fname for fname in os.listdir(output_path) if "trace_org_" in fname]
    def max_key(s):
        u = int(s.split("_update_")[-1].split(".")[0])
        if update == None:
            return u
        else:
            return u if u <= update else -1
    return os.path.join(output_path, max(trace_files, key=max_key))


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
    run_dirs = [os.path.join(data_dir, run_dir) for data_dir in data_dirs for run_dir in os.listdir(data_dir) if "__SEED_" in run_dir]

    # sort run directories by seed to make easier on the eyes
    run_dirs.sort(key=lambda x : int(x.split("_")[-1]))
    print(f"Found {len(run_dirs)} run directories.")
    analysis_header_set = set() # Use this to guarantee all organism file headers match.
    # For each run, aggregate max fitness organism information.
    analysis_org_infos = []
    for run in run_dirs:
        print(f"Extracting information from {run}")
        run_config_path = os.path.join(run, "output", "run_config.csv")
        # these find functions will crash
        org_analysis_path = find_org_analysis_path(run, update if update >= 0 else None)
        org_trace_path = find_trace_path(run, update if update >= 0 else None)
        if not os.path.exists(run_config_path):
            print(f"Failed to find run parameters ({run_config_path})")
            exit(-1)
        # double check that analysis and trace files are from the same update and org ID
        analysis_update = org_analysis_path.split("/")[-1].split("_update_")[-1].split(".")[0]
        trace_update = org_trace_path.split("/")[-1].split("_update_")[-1].split(".")[0]
        if analysis_update != trace_update:
            print(f"Analysis file and trace file updates do not match: \n  * {analysis_update}\n  * {trace_update}\n")
            exit(-1)
        analysis_id = org_analysis_path.split("/")[-1].strip("analysis_org_").split()
        trace_id = org_trace_path.split("/")[-1].strip("trace_org_").split()
        if analysis_id != trace_id:
            print(f"Analysis file and trace file updates do not match: \n  * {analysis_id}\n  * {trace_id}\n")
            exit(-1)

        # extract run settings
        run_settings = extract_settings(run_config_path)

        # ========= extract analysis file info =========
        content = None
        with open(org_analysis_path, "r") as fp:
            content = fp.read().strip().split("\n")
        analysis_header = content[0].split(",")
        analysis_header_lu = {analysis_header[i].strip():i for i in range(0, len(analysis_header))}
        content = content[1:]
        orgs = [l for l in csv.reader(content, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True)]
        org = orgs[-1]

        # -- collect extra fields --
        base_score = float(org[analysis_header_lu["score"]])
        ko_reg_score = float(org[analysis_header_lu["score__ko_regulation"]])
        ko_mem_score = float(org[analysis_header_lu["score__ko_global_memory"]])
        ko_msg_score = float(org[analysis_header_lu["score__ko_messaging"]])
        ko_imprinting_score = float(org[analysis_header_lu["score__ko_regulation_imprinting"]])
        ko_repro_tag_score = float(org[analysis_header_lu["score__ko_reproduction_tag_control"]])

        uses_reg = int(ko_reg_score < base_score)
        uses_mem = int(ko_mem_score < base_score)
        uses_msg = int(ko_msg_score < base_score)
        uses_imprinting = int(ko_imprinting_score < base_score)
        uses_repro_tag = int(ko_repro_tag_score < base_score)

        ko_reg_delta = base_score - ko_reg_score
        ko_mem_delta = base_score - ko_mem_score
        ko_msg_delta = base_score - ko_msg_score
        ko_imprinting_delta = base_score - ko_imprinting_score
        ko_repro_tag_delta = base_score - ko_repro_tag_score


        extra_fields = [
            "relies_on_reg",
            "relies_on_mem",
            "relies_on_msg",
            "relies_on_imprinting",
            "relies_on_repro_tag",
            "ko_reg_delta",
            "ko_mem_delta",
            "ko_msg_delta",
            "ko_imprinting_delta",
            "ko_repro_tag_delta"
        ]

        extra_values = [
            uses_reg,
            uses_mem,
            uses_msg,
            uses_imprinting,
            uses_repro_tag,
            ko_reg_delta,
            ko_mem_delta,
            ko_msg_delta,
            ko_imprinting_delta,
            ko_repro_tag_delta
        ]

        analysis_header_set.add(",".join([key for key in key_settings] + extra_fields + analysis_header))
        if len(analysis_header_set) > 1:
            print(f"Header mismatch! ({org_analysis_path})")
            exit(-1)
        # surround things in quotes that need it
        org[analysis_header_lu["program"]] = "\"" + org[analysis_header_lu["program"]] + "\""
        for field in analysis_header_lu:
            if not len(org[analysis_header_lu[field]]): continue
            if org[analysis_header_lu[field]][0] == "[":
                org[analysis_header_lu[field]] = "\"" + org[analysis_header_lu[field]] + "\""

        # append csv line (as a list) for analysis orgs
        analysis_org_infos.append([run_settings[key] for key in key_settings] + extra_values + org)

    # Output analysis org infos
    out_content = list(analysis_header_set)[0] + "\n" # Should be guaranteed to be length 1!
    out_content += "\n".join([",".join(map(str, line)) for line in analysis_org_infos])
    with open(os.path.join(dump_dir, dump_fname), "w") as fp:
        fp.write(out_content)
    print(f"Done! Output written to {os.path.join(dump_dir, dump_fname)}")

if __name__ == "__main__":
    main()