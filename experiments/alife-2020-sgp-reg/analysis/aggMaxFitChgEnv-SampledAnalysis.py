'''
Script: aggMaxFit.py
For each run, grab the maximum fitness organism at end of run.
'''

import argparse, os, copy, errno, csv, re, sys
import hjson,json

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
    "TOURNAMENT_SIZE"
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
        max_fit_path = os.path.join(run, "output", "max_fit_org.csv")
        if not os.path.exists(run_config_path):
            print(f"Failed to find run parameters ({run_config_path})")
            exit(-1)
        # double check that analysis and trace files are from the same update and org ID
        analysis_update = org_analysis_path.split("/")[-1].split("_update_")[-1].split(".")[0]
        trace_update = org_trace_path.split("/")[-1].split("_update_")[-1].split(".")[0]
        if analysis_update != trace_update:
            print(f"Analysis file and trace file updates do not match: \n  * {analysis_update}\n  * {trace_update}\n")
            exit(-1)
        analysis_id = org_analysis_path.split("/")[-1].split("org_")[-1].split("_")[0]
        trace_id = org_trace_path.split("/")[-1].split("org_")[-1].split("_")[0]
        if analysis_id != trace_id:
            print(f"Analysis file and trace file updates do not match: \n  * {analysis_id}\n  * {trace_id}\n")
            exit(-1)

        # extract run settings
        run_settings = extract_settings(run_config_path)

        # ========= extract analysis file info =========
        content = None
        with open(max_fit_path, "r") as fp:
            content = fp.read().strip().split("\n")
        max_fit_header = content[0].split(",")
        max_fit_header_lu = {max_fit_header[i].strip():i for i in range(0, len(max_fit_header))}
        content = content[1:]
        orgs = [l for l in csv.reader(content, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True)]
        max_fit_org = orgs[-1]
        max_fit_id = max_fit_org[max_fit_header_lu["pop_id"]]
        if max_fit_id != analysis_id:
            print("Max fit id: ", max_fit_id)
            print("Analysis id: ", analysis_id)
            print("Something's gone WRONG!")
            exit(-1)

        # Fields to collect from max fit file.
        max_fit_vals = {
            "update": max_fit_org[max_fit_header_lu["update"]],
            "solution": max_fit_org[max_fit_header_lu["solution"]],
            "score": max_fit_org[max_fit_header_lu["score"]],
            "num_matches": max_fit_org[max_fit_header_lu["num_matches"]],
            "num_misses": max_fit_org[max_fit_header_lu["num_misses"]],
            "num_no_responses": max_fit_org[max_fit_header_lu["num_no_responses"]]
        }
        max_fit_fields = ["update","solution","score","num_matches","num_misses","num_no_responses"]

        # ========= extract analysis file info =========
        content = None
        with open(org_analysis_path, "r") as fp:
            content = fp.read().strip().split("\n")
        analysis_header = content[0].split(",")
        analysis_header_lu = {analysis_header[i].strip():i for i in range(0, len(analysis_header))}
        content = content[1:]
        analysis_orgs = [l for l in csv.reader(content, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True)]

        num_analyses = len(analysis_orgs)
        num_analysis_solutions = 0
        num_analysis_solutions_ko_reg = 0
        num_analysis_solutions_ko_mem = 0
        num_analysis_solutions_ko_all = 0

        solution_score = float(run_settings["NUM_ENV_UPDATES"])

        all_solution = True
        all_solution_ko_reg = True
        all_solution_ko_mem = True
        all_solution_ko_all = True

        for analysis_org in analysis_orgs:
            org_info = { analysis_header[i]:analysis_org[i] for i in range(0, len(analysis_org)) }
            base_score = float(org_info["score"])
            ko_reg_score = float(org_info["score_ko_regulation"])
            ko_mem_score = float(org_info["score_ko_global_memory"])
            ko_all_score = float(org_info["score_ko_all"])

            is_sol = base_score >= solution_score
            ko_reg_is_sol = ko_reg_score >= solution_score
            ko_mem_is_sol = ko_mem_score >= solution_score
            ko_all_is_sol = ko_all_score >= solution_score

            if (is_sol):
                num_analysis_solutions += 1
            else:
                all_solution = False
            if (ko_reg_is_sol):
                num_analysis_solutions_ko_reg += 1
            else:
                all_solution_ko_reg = False
            if (ko_mem_is_sol):
                num_analysis_solutions_ko_mem += 1
            else:
                all_solution_ko_mem = False
            if (ko_all_is_sol):
                num_analysis_solutions_ko_all += 1
            else:
                all_solution_ko_all = False
        # end analysis file for loop
        analysis_vals = {
            "num_analyses": num_analyses,
            "num_analysis_solutions": num_analysis_solutions,
            "num_analysis_solutions_ko_reg": num_analysis_solutions_ko_reg,
            "num_analysis_solutions_ko_mem": num_analysis_solutions_ko_mem,
            "num_analysis_solutions_ko_all": num_analysis_solutions_ko_all,
            "all_solution": int(all_solution),
            "all_solution_ko_reg": int(all_solution_ko_reg),
            "all_solution_ko_mem": int(all_solution_ko_mem),
            "all_solution_ko_all": int(all_solution_ko_all)
        }
        analysis_fields=["num_analyses","num_analysis_solutions","num_analysis_solutions_ko_reg","num_analysis_solutions_ko_mem","num_analysis_solutions_ko_all","all_solution","all_solution_ko_reg","all_solution_ko_mem","all_solution_ko_all"]

        analysis_header_set.add(",".join([key for key in key_settings] + max_fit_fields + analysis_fields))
        if len(analysis_header_set) > 1:
            print(f"Header mismatch! ({org_analysis_path})")
            exit(-1)
        # # surround things in quotes that need it
        # org[analysis_header_lu["program"]] = "\"" + org[analysis_header_lu["program"]] + "\""
        analysis_org_infos.append([run_settings[key] for key in key_settings] + [max_fit_vals[field] for field in max_fit_fields] + [analysis_vals[field] for field in analysis_fields])

    # Output analysis org infos
    out_content = list(analysis_header_set)[0] + "\n" # Should be guaranteed to be length 1!
    out_content += "\n".join([",".join(map(str, line)) for line in analysis_org_infos])
    with open(os.path.join(dump_dir, dump_fname), "w") as fp:
        fp.write(out_content)
    print(f"Done! Output written to {os.path.join(dump_dir, dump_fname)}")

if __name__ == "__main__":
    main()