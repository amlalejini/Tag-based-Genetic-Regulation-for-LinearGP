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
    "NUM_SIGNAL_RESPONSES",
    "NUM_ENV_CYCLES",
    "USE_FUNC_REGULATION",
    "USE_GLOBAL_MEMORY",
    "MUT_RATE__INST_TAG_BF",
    "MUT_RATE__FUNC_TAG_BF",
    "CPU_TIME_PER_ENV_CYCLE",
    "MAX_FUNC_CNT",
    "MAX_FUNC_INST_CNT",
    "MAX_ACTIVE_THREAD_CNT",
    "MAX_THREAD_CAPACITY",
    "TOURNAMENT_SIZE"
]

possible_metrics = ["hamming", "streak", "symmetric wrap", "hash"]
possible_selectors = ["ranked"]
possible_regulators = ["add", "mult"]

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
        ko_reg_score = float(org[analysis_header_lu["score_ko_regulation"]])
        ko_gmem_score = float(org[analysis_header_lu["score_ko_global_memory"]])
        ko_all_score = float(org[analysis_header_lu["score_ko_all"]])
        use_regulation = int(ko_reg_score < base_score)
        use_global_memory = int(ko_gmem_score < base_score)
        use_either = int(ko_all_score < base_score)
        ko_reg_delta = base_score - ko_reg_score
        ko_global_mem_delta = base_score - ko_gmem_score
        ko_all_delta = base_score - ko_all_score
        extra_fields = ["relies_on_regulation", "relies_on_global_memory", "relies_on_either",
                        "ko_regulation_delta", "ko_global_memory_delta", "ko_all_delta"]
        extra_values = [use_regulation, use_global_memory, use_either,
                        ko_reg_delta, ko_global_mem_delta, ko_all_delta]

        analysis_header_set.add(",".join([key for key in key_settings] + extra_fields + analysis_header))
        if len(analysis_header_set) > 1:
            print(f"Header mismatch! ({org_analysis_path})")
            exit(-1)
        # surround things in quotes that need it
        org[analysis_header_lu["program"]] = "\"" + org[analysis_header_lu["program"]] + "\""
        num_modules = int(org[analysis_header_lu["num_modules"]])
        # org_update = int(org[analysis_header_lu["update"]])
        # append csv line (as a list) for analysis orgs
        analysis_org_infos.append([run_settings[key] for key in key_settings] + extra_values + org)

        # ========= extract org trace information =========
        content = None
        with open(org_trace_path, "r") as fp:
            content = fp.read().strip().split("\n")
        trace_header = content[0].split(",")
        trace_header_lu = {trace_header[i].strip():i for i in range(0, len(trace_header))}
        content = content[1:]
        steps = [l for l in csv.reader(content, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True)]
        time_steps = [i for i in range(len(steps))]

        # We want to work with the thread state info as a python dict, so..
        #  - raw string ==[hjson]==> OrderedDict ==[json]==> python dict
        thread_states = [step[trace_header_lu["thread_state_info"]].replace(",", ",\n") for step in steps]
        thread_states = [list(json.loads(json.dumps(hjson.loads(state)))) for state in thread_states]
        if len(set([len(thread_states), len(time_steps), len(steps)])) != 1:
            print("Trace steps do not match among components.")
            exit(-1)

        # From thread states, collect lists: currently_running,
        # currently_active = [] # For each time step, which modules are currently active?
        # currently_running = []
        modules_run_in_env_cycle = [set() for i in range(int(run_settings["NUM_ENV_CYCLES"]))]
        match_delta_in_env_cycle = [[0 for i in range(num_modules)] for i in range(int(run_settings["NUM_ENV_CYCLES"]))]
        module_triggered_by_env_cycle = [None for i in range(int(run_settings["NUM_ENV_CYCLES"]))]
        modules_active_ever = set()
        modules_present_by_step = [[0 for m in range(0, num_modules)] for i in range(0, len(steps))]
        modules_active_by_step = [[0 for m in range(0, num_modules)] for i in range(0, len(steps))]

        cur_env = int(steps[0][trace_header_lu["env_cycle"]])
        baseline_match_scores = list(map(float, steps[0][trace_header_lu["env_signal_match_scores"]].strip("[]").split(",")))
        module_triggered_by_env_cycle[0] = steps[0][trace_header_lu["env_signal_closest_match"]]
        for i in range(0, len(steps)):
            # print(f"==== step {i} ====")
            step_info = steps[i]
            threads = thread_states[i]
            # Extract current env cycle
            env_cycle = int(step_info[trace_header_lu["env_cycle"]])
            # If we're in a new env cycle, update the regulation delta for the
            if env_cycle != cur_env:
                cur_match_scores = list(map(float, step_info[trace_header_lu["env_signal_match_scores"]].strip("[]").split(",")))
                match_delta_in_env_cycle[cur_env] = [cur - baseline for baseline, cur in zip(baseline_match_scores, cur_match_scores)]
                # update baseline/current environment
                baseline_match_scores = cur_match_scores
                cur_env = env_cycle
                module_triggered_by_env_cycle[env_cycle] = step_info[trace_header_lu["env_signal_closest_match"]]
            # Extract what modules are running
            # print("# Threads = ", len(threads))
            # print(threads)
            active_modules = []
            present_modules = []
            for thread in threads:
                call_stack = thread["call_stack"]
                # an active module is at the top of the flow stack on the top of the call stack
                active_module = None
                if len(call_stack):
                    if len(call_stack[-1]["flow_stack"]):
                        active_module = call_stack[-1]["flow_stack"][-1]["mp"]
                if active_module != None:
                    active_modules.append(active_module)
                    modules_active_ever.add(active_module)
                # add ALL modules
                present_modules += list({flow["mp"] for call in call_stack for flow in call["flow_stack"]})
            # add present modules to env set for this env
            for mod_id in present_modules: modules_run_in_env_cycle[env_cycle].add(int(mod_id))
            # Add active modules for this step
            for module_id in active_modules: modules_active_by_step[i][module_id] += 1
            # Add present modules for this step
            for module_id in present_modules: modules_present_by_step[i][module_id] += 1
            # update
            final_match_scores = list(map(float, steps[-1][trace_header_lu["env_signal_match_scores"]].strip("[]").split(",")))
            match_delta_in_env_cycle[cur_env] = [baseline - final for baseline, final in zip(baseline_match_scores, final_match_scores)]

        # ========= build trace out file for this run =========
        # - There's one trace output file per run
        # - Extra fields:
        #   - module_id
        #   - time_step
        #   - currently_running/in the call stack
        #   - match_score__mid_[:]
        #   - regulator_state__mid_[:]
        trace_out_name = f"trace_update-{update}_run-id-" + run_settings["SEED"] + ".csv"
        orig_fields = ["env_cycle","cpu_step","num_env_states","cur_env_state","cur_response","has_correct_response","num_modules","env_signal_closest_match","num_active_threads"]
        derived_fields = ["module_id", "time_step", "is_in_call_stack", "is_running", "is_match", "is_ever_active", "match_score", "regulator_state"]
        trace_header = ",".join(orig_fields + derived_fields)
        trace_out_lines = []
        expected_lines = num_modules * len(steps)
        # For each time step, for each module => add line to file
        for step_i in range(0, len(steps)):
            step_info = steps[step_i]
            orig_component = [step_info[trace_header_lu[field]] for field in orig_fields]
            time_step = time_steps[step_i]
            cur_match = int(step_info[trace_header_lu["env_signal_closest_match"]])
            cur_match_scores = list(map(float, step_info[trace_header_lu["env_signal_match_scores"]].strip("[]").split(",")))
            cur_reg_states = list(map(float, step_info[trace_header_lu["module_regulator_states"]].strip("[]").split(",")))
            modules_present = modules_present_by_step[step_i]
            modules_active = modules_active_by_step[step_i]
            for module_id in range(0, num_modules):
                derived_vals = [module_id,                    # module_id
                                time_step,                    # time_step
                                modules_present[module_id],   # is_in_call_stack
                                modules_active[module_id],    # is_running
                                int(cur_match == module_id),  # is_match
                                int(module_id in modules_active_ever), # is_ever_active
                                cur_match_scores[module_id],  # match_score
                                cur_reg_states[module_id]     # regulator_state
                               ]
                trace_out_lines.append(",".join(map(str, orig_component + derived_vals)))
        if expected_lines != len(trace_out_lines):
            print("AAAAAHHHHH!")
            exit(-1)
        with open(os.path.join(dump_dir, trace_out_name), "w") as fp:
            fp.write("\n".join([trace_header] + trace_out_lines))
        print("  Wrote out:", os.path.join(dump_dir, trace_out_name))

        # ========= build environment cycle regulation graph =========
        # num_env_cycles = int(run_settings["NUM_ENV_CYCLES"])
        # env_cycle_graph_out_name = f"env-cycle-graph_update-{update}_run-id-" + run_settings["SEED"] + ".csv"
        # env_cycle_graph_fields = ["env_cycle", "module_triggered", "modules_run", "match_delta", "promoted", "repressed", "unchanged"]
        # lines = [",".join(env_cycle_graph_fields)]
        # for cycle in range(0, num_env_cycles):
        #     module_triggered = module_triggered_by_env_cycle[cycle]
        #     match_deltas = match_delta_in_env_cycle[cycle]
        #     # What modules were running?
        #     modules_run = "\"" + str(modules_run_in_env_cycle[cycle]).replace(" ","") + "\""
        #     # What was promoted?
        #     promoted = "\"" + str([mod_id for mod_id in range(0, num_modules) if match_deltas[mod_id] < 0]).replace(" ", "") + "\""
        #     # What was repressed?
        #     repressed = "\"" + str([mod_id for mod_id in range(0, num_modules) if match_deltas[mod_id] > 0]).replace(" ", "") + "\""
        #     # What was unchanged?
        #     unchanged = "\"" + str([mod_id for mod_id in range(0, num_modules) if match_deltas[mod_id] == 0]).replace(" ", "") + "\""
        #     deltas = "\"" + str(match_deltas).replace(" ", "") + "\""
        #     line = ",".join([str(cycle), str(module_triggered), modules_run, deltas, promoted, repressed, unchanged])
        #     lines.append(line)
        # with open(os.path.join(dump_dir, env_cycle_graph_out_name), "w") as fp:
        #     fp.write("\n".join(lines))
        # print("  Wrote out:", os.path.join(dump_dir, env_cycle_graph_out_name))

        # num_env_cycles = int(run_settings["NUM_ENV_CYCLES"])
        env_cycle_graph_out_name = f"reg-graph_update-{update}_run-id-" + run_settings["SEED"] + ".csv"
        env_cycle_graph_fields = ["state_id", "env_cycle", "time_step", "module_triggered", "active_modules", "promoted", "repressed", "match_scores" , "match_deltas"]
        lines = [",".join(env_cycle_graph_fields)]

        state_i = None
        prev_env_cycle = None
        prev_active_modules = None
        prev_match_scores = None
        found_first_module = False

        for step_i in range(0, len(steps)):
            step_info = steps[step_i]
            # Extract current env cycle
            env_cycle = int(step_info[trace_header_lu["env_cycle"]])
            # Extract active modules (top of call stacks)
            active_modules = {i for i in range(0, num_modules) if modules_active_by_step[step_i][i] > 0}
            match_scores = list(map(float, step_info[trace_header_lu["env_signal_match_scores"]].strip("[]").split(",")))

            # if this is the first time step, setup 'previous' state
            if step_i == 0:
                state_i = 0
                prev_env_cycle = env_cycle
                prev_active_modules = active_modules
                prev_match_scores = match_scores
            if not found_first_module:
                prev_active_modules = active_modules
                found_first_module = len(active_modules) > 0

            # Has anything been repressed/promoted?
            match_deltas = [match_scores[i] - prev_match_scores[i] for i in range(0, num_modules)]
            repressed_modules = {mod_id for mod_id in range(0, num_modules) if match_deltas[mod_id] < 0}
            promoted_modules = {mod_id for mod_id in range(0, num_modules) if match_deltas[mod_id] > 0}

            # if current active modules or current env cycle don't match previous, output what happened since last time we output
            if (active_modules != prev_active_modules and len(active_modules) != 0):
                promoted_str = "\"" + str(list(promoted_modules)).replace(" ", "") + "\""
                repressed_str = "\"" + str(list(repressed_modules)).replace(" ", "") + "\""
                deltas = "\"" + str(match_deltas).replace(" ", "") + "\"" # deltas from beginning => end of state
                scores = "\"" + str(match_scores).replace(" ", "") + "\"" # match scores at end of state
                active_modules_str = "\"" + str(list(prev_active_modules)).replace(" ", "") + "\"" # active modules _during_ this state
                module_triggered = module_triggered_by_env_cycle[prev_env_cycle] # module triggered for this environment cycle

                lines.append(",".join([str(state_i), str(prev_env_cycle), str(step_i), str(module_triggered), active_modules_str, promoted_str, repressed_str, scores, deltas]))

                prev_match_scores = match_scores
                prev_active_modules = active_modules
                prev_env_cycle = env_cycle
                state_i += 1

        with open(os.path.join(dump_dir, env_cycle_graph_out_name), "w") as fp:
            fp.write("\n".join(lines))
        print("  Wrote out:", os.path.join(dump_dir, env_cycle_graph_out_name))

    # Output analysis org infos
    out_content = list(analysis_header_set)[0] + "\n" # Should be guaranteed to be length 1!
    out_content += "\n".join([",".join(map(str, line)) for line in analysis_org_infos])
    with open(os.path.join(dump_dir, dump_fname), "w") as fp:
        fp.write(out_content)
    print(f"Done! Output written to {os.path.join(dump_dir, dump_fname)}")

if __name__ == "__main__":
    main()