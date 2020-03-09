'''
Script: aggMaxFit.py
For each run, grab the maximum fitness organism at end of run.

:horror:
This script is a result of code sprinting. It desperately needs to be cleaned up.
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
    "MUT_RATE__FUNC_DUP"
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
        org_trace_dir = find_trace_dir(run, update if update >= 0 else None)
        if not os.path.exists(run_config_path):
            print(f"Failed to find run parameters ({run_config_path})")
            exit(-1)
        # double check that analysis and trace files are from the same update and org ID
        analysis_update = org_analysis_path.split("/")[-1].split("_update_")[-1].split(".")[0]
        trace_update = org_trace_dir.split("/")[-1].split("-")[-1]
        if analysis_update != trace_update:
            print(f"Analysis file and trace file updates do not match: \n  * {analysis_update}\n  * {trace_update}\n")
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
        base_score = float(org[analysis_header_lu["aggregate_score"]])
        ko_reg_score = float(org[analysis_header_lu["ko_regulation_aggregate_score"]])
        ko_gmem_score = float(org[analysis_header_lu["ko_global_memory_aggregate_score"]])
        ko_all_score = float(org[analysis_header_lu["ko_all_aggregate_score"]])
        ko_up_reg_score = float(org[analysis_header_lu["ko_up_reg_aggregate_score"]])
        ko_down_reg_score = float(org[analysis_header_lu["ko_down_reg_aggregate_score"]])
        use_regulation = int(ko_reg_score < base_score)
        use_global_memory = int(ko_gmem_score < base_score)
        use_either = int(ko_all_score < base_score)
        relies_on_up_reg = int(ko_up_reg_score < base_score)
        relies_on_down_reg = int(ko_down_reg_score < base_score)
        ko_reg_delta = base_score - ko_reg_score
        ko_global_mem_delta = base_score - ko_gmem_score
        ko_all_delta = base_score - ko_all_score
        ko_up_reg_delta = base_score - ko_up_reg_score
        ko_down_reg_delta = base_score - ko_down_reg_score
        extra_fields = ["relies_on_regulation", "relies_on_global_memory", "relies_on_either",
                        "relies_on_up_reg", "relies_on_down_reg",
                        "ko_regulation_delta", "ko_global_memory_delta", "ko_all_delta",
                        "ko_up_reg_delta", "ko_down_reg_delta"]
        trace_fields = ["call_promoted_cnt", "call_repressed_cnt"]
        extra_values = [use_regulation, use_global_memory, use_either,
                        relies_on_up_reg, relies_on_down_reg,
                        ko_reg_delta, ko_global_mem_delta, ko_all_delta,
                        ko_up_reg_delta, ko_down_reg_delta]

        analysis_header_set.add(",".join([key for key in key_settings] + extra_fields + trace_fields + analysis_header))
        if len(analysis_header_set) > 1:
            print(f"Header mismatch! ({org_analysis_path})")
            exit(-1)
        # surround things in quotes that need it
        # org[analysis_header_lu["program"]] = "\"" + org[analysis_header_lu["program"]] + "\""
        org[analysis_header_lu["program"]] = "NONE"
        org[analysis_header_lu["scores_by_test"]] = "\"" + org[analysis_header_lu["scores_by_test"]] + "\""
        org[analysis_header_lu["ko_regulation_scores_by_test"]] = "\"" + org[analysis_header_lu["ko_regulation_scores_by_test"]] + "\""
        org[analysis_header_lu["ko_global_memory_scores_by_test"]] = "\"" + org[analysis_header_lu["ko_global_memory_scores_by_test"]] + "\""
        org[analysis_header_lu["ko_all_scores_by_test"]] = "\"" + org[analysis_header_lu["ko_all_scores_by_test"]] + "\""
        org[analysis_header_lu["ko_up_reg_scores_by_test"]] = "\"" + org[analysis_header_lu["ko_up_reg_scores_by_test"]] + "\""
        org[analysis_header_lu["ko_down_reg_scores_by_test"]] = "\"" + org[analysis_header_lu["ko_down_reg_scores_by_test"]] + "\""
        org[analysis_header_lu["test_ids"]] = "\"" + org[analysis_header_lu["test_ids"]] + "\""
        org[analysis_header_lu["test_seqs"]] = "\"" + org[analysis_header_lu["test_seqs"]] + "\""
        num_modules = int(org[analysis_header_lu["num_modules"]])

        # TODO - this whole bit should be => for each trace file... extract
        trace_out_name = f"trace_update-{update}_run-id-" + run_settings["SEED"] + ".csv"
        orig_trace_fields = ["env_seq","test_id","num_env_states","env_update","cpu_step","cur_env_state","direction","cur_env_signal_tag","has_response","cur_response","has_correct_response","num_modules","0_signal_closest_match","1_signal_closest_match","num_active_threads"]
        derived_trace_fields = ["module_id", "time_step", "is_in_call_stack", "is_running", "is_cur_responding_module", "is_match_cur_dir","is_match_0","is_match_1","is_ever_active", "0_match_score", "1_match_score", "regulator_state"]
        trace_content_lines = []
        out_trace_header = ",".join(orig_trace_fields + derived_trace_fields)
        trace_files = sorted([fname for fname in os.listdir(org_trace_dir)])
        repressing_calls = 0
        promoting_calls = 0

        reg_graph_out_name = f"reg-graph_update-{update}_run-id-" + run_settings["SEED"] + ".csv"
        reg_graph_fields = ["test_id","test_seq","state_id","env_update","time_step","module_triggered", "module_responded", "active_modules","promoted","repressed","reg_deltas"]
        reg_lines = [",".join(reg_graph_fields)]

        for trace_file in trace_files:
            trace_path = os.path.join(org_trace_dir, trace_file)
            content = None
            with open(trace_path, "r") as fp:
                content = fp.read().strip().split("\n")
            trace_header = content[0].split(",")
            trace_header_lu = {trace_header[i].strip():i for i in range(0, len(trace_header))}
            content = content[1:]
            steps = [l for l in csv.reader(content, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True)]
            time_steps = [i for i in range(len(steps))]

            thread_states = [step[trace_header_lu["thread_state_info"]].replace(",", ",\n") for step in steps]
            thread_states = [list(json.loads(json.dumps(hjson.loads(state)))) for state in thread_states]
            if len(set([len(thread_states), len(time_steps), len(steps)])) != 1:
                print("Trace steps do not match among components.")
                exit(-1)

            num_env_cycles = int(run_settings["NUM_ENV_UPDATES"])
            num_env_states = int(run_settings["NUM_ENV_STATES"])
            # Extract information from trace file (by-step, by-env-cycle)
            modules_active_ever = set()
            modules_run_in_env_cycle = [set() for i in range(num_env_cycles)]
            match_delta_in_env_cycle = [ {"left": [0 for i in range(num_modules)], "right": [0 for i in range(num_modules)]} for i in range(num_env_cycles)]
            direction_by_env_cycle = [None for i in range(num_env_cycles)]
            module_triggered_by_env_cycle = [None for i in range(num_env_cycles)]
            module_response_by_env_cycle = [set() for i in range(num_env_cycles)]
            modules_present_by_step = [[0 for m in range(0, num_modules)] for i in range(0, len(steps))]
            modules_active_by_step = [[0 for m in range(0, num_modules)] for i in range(0, len(steps))]
            modules_triggered_by_step = [None for i in range(0, len(steps))]
            modules_responded_by_step = [None for i in range(0, len(steps))]

            cur_env_update = int(steps[0][trace_header_lu["env_update"]])
            cur_env_state = int(steps[0][trace_header_lu["cur_env_state"]])
            cur_direction = steps[0][trace_header_lu["direction"]]
            trace_test_id = steps[0][trace_header_lu["test_id"]]
            trace_test_seq = steps[0][trace_header_lu["env_seq"]]

            baseline_match_scores_left = list(map(float, steps[0][trace_header_lu["0_signal_match_scores"]].strip("[]").split(",")))
            baseline_match_scores_right = list(map(float, steps[0][trace_header_lu["1_signal_match_scores"]].strip("[]").split(",")))

            # 0_signal_closest_match
            # 1_signal_closest_match
            module_triggered_by_env_cycle[0] = steps[0][trace_header_lu[cur_direction + "_signal_closest_match"]]
            modules_triggered_by_step[0] = int(steps[0][trace_header_lu[cur_direction + "_signal_closest_match"]])
            # Figure out which module responded to each environment signal.
            for i in range(0, len(steps)):
                step_info = {trace_header[j]: steps[i][j] for j in range(0, len(steps[i])) }
                cur_response_module_id = int(step_info["cur_responding_function"])
                cur_env_update = int(step_info["env_update"])
                if cur_response_module_id != -1:
                    module_response_by_env_cycle[cur_env_update].add(cur_response_module_id)
                    modules_responded_by_step[i] = cur_response_module_id
            if any([len(e) > 1 for e in module_response_by_env_cycle]):
                print("something bad")
                exit(-1)

            # Loop over each step of the trace, collecting information as we go.
            for i in range(0, len(steps)):
                step_info = {trace_header[j]: steps[i][j] for j in range(0, len(steps[i])) }
                threads = thread_states[i]
                # Extract current environment update
                env_update_i = int(step_info["env_update"])
                env_state_i = step_info["cur_env_state"]
                dir_i = step_info["direction"]
                # If we're in a new env update, update the regulation delta.
                if cur_env_update != env_update_i:
                    left_match_scores_i = list(map(float, step_info["0_signal_match_scores"].strip("[]").split(",")))
                    right_match_scores_i = list(map(float, step_info["1_signal_match_scores"].strip("[]").split(",")))
                    match_delta_in_env_cycle[cur_env_update]["left"] = [cur - baseline for baseline, cur in zip(baseline_match_scores_left, left_match_scores_i)]
                    match_delta_in_env_cycle[cur_env_update]["right"] = [cur - baseline for baseline, cur in zip(baseline_match_scores_right, right_match_scores_i)]
                    # update baselines
                    cur_direction = dir_i
                    cur_env_update = env_update_i
                    cur_env_state = env_state_i
                    module_triggered_by_env_cycle[env_update_i] = step_info[dir_i + "_signal_closest_match"]
                    modules_triggered_by_step[i] = step_info[dir_i + "_signal_closest_match"]
                # Extract what modules are running
                active_modules = []
                present_modules = []
                if modules_triggered_by_step[i] != None:
                    active_modules.append(int(modules_triggered_by_step[i]))
                    present_modules.append(int(modules_triggered_by_step[i]))
                if modules_responded_by_step[i] != None:
                    active_modules.append(int(modules_responded_by_step[i]))
                    present_modules.append(int(modules_responded_by_step[i]))
                for thread in threads:
                    call_stack = thread["call_stack"]
                    # an active module is at the top of the TOP flow stack (i.e., in top call)
                    active_module = None
                    if len(call_stack):
                        if len(call_stack[-1]["flow_stack"]):
                            active_module = call_stack[-1]["flow_stack"][-1]["mp"]
                    if active_module != None:
                        active_modules.append(int(active_module))
                        modules_active_ever.add(int(active_module))
                    # add ALL modules
                    present_modules += list({flow["mp"] for call in call_stack for flow in call["flow_stack"]})
                # add present modules to env set for this env
                for module_id in present_modules: modules_run_in_env_cycle[env_update_i].add(int(module_id))
                # Add active modules for this step
                for module_id in active_modules: modules_active_by_step[i][module_id] += 1
                # Add present modules for this step
                for module_id in present_modules: modules_present_by_step[i][module_id] += 1
            # update final env update deltas
            left_final_match_scores = list(map(float, steps[-1][trace_header_lu["0_signal_match_scores"]].strip("[]").split(",")))
            right_final_match_scores = list(map(float, steps[-1][trace_header_lu["1_signal_match_scores"]].strip("[]").split(",")))
            match_delta_in_env_cycle[cur_env_update]["left"] = [final - baseline for baseline, final in zip(baseline_match_scores_left, left_final_match_scores)]
            match_delta_in_env_cycle[cur_env_update]["right"] = [final - baseline for baseline, final in zip(baseline_match_scores_right, right_final_match_scores)]

            # ====== Build trace output file for this run =====
            expected_lines = num_modules * len(steps)
            line_cnt = 0
            # trace_content_lines
            # derived fields: ["module_id", "time_step", "is_in_call_stack", "is_running", "is_match_cur_dir","is_match_0","is_match_1","is_ever_active", "0_match_score", "1_match_score", "regulator_state"]
            for step_i in range(0, len(steps)):
                step_info = step_info = {trace_header[j]: steps[step_i][j] for j in range(0, len(steps[step_i])) }
                original_trace_content = [step_info[field] for field in orig_trace_fields]
                time_step = time_steps[step_i]
                cur_direction = step_info["direction"]
                cur_responding_function = int(step_info["cur_responding_function"])
                cur_left_match = int(step_info["0_signal_closest_match"])
                cur_right_match = int(step_info["1_signal_closest_match"])
                cur_left_match_scores = list(map(float, step_info["0_signal_match_scores"].strip("[]").split(",")))
                cur_right_match_scores = list(map(float, step_info["1_signal_match_scores"].strip("[]").split(",")))
                cur_reg_states = list(map(float, step_info["module_regulator_states"].strip("[]").split(",")))
                modules_in_call_stack = modules_present_by_step[step_i]
                modules_active = modules_active_by_step[step_i]
                for module_id in range(0, num_modules):
                    is_match_dir = False
                    if cur_direction == "0":
                        is_match_dir = cur_left_match == module_id
                    elif cur_direction == "1":
                        is_match_dir = cur_right_match == module_id
                    else:
                        # something bad happened
                        print("BAD DIRECTION")
                        exit(-1)
                    derived_vals = {
                        "module_id": module_id,
                        "time_step": time_step,
                        "is_in_call_stack": modules_in_call_stack[module_id],
                        "is_running": modules_active[module_id],
                        "is_cur_responding_module": int(cur_responding_function == module_id),
                        "is_match_cur_dir": int(is_match_dir),
                        "is_match_0": int(cur_left_match == module_id),
                        "is_match_1": int(cur_right_match == module_id),
                        "is_ever_active": int(module_id in modules_active_ever),
                        "0_match_score": cur_left_match_scores[module_id],
                        "1_match_score": cur_right_match_scores[module_id],
                        "regulator_state": cur_reg_states[module_id]
                    }
                    derived_content = [derived_vals[field] for field in derived_trace_fields]
                    trace_content_lines.append(",".join(map(str, original_trace_content + derived_content)))
                    line_cnt += 1
            if expected_lines != line_cnt:
                print("AHHH!")
                exit(-1)
            # pass
            # ========= build environment cycle regulation graph =========

            state_i = None
            prev_env_update = None
            prev_active_modules = None
            prev_match_scores_left = None
            prev_match_scores_right = None
            prev_reg_state = None
            found_first_module = False

            for step_i in range(0, len(steps)):
                step_info = step_info = {trace_header[j]: steps[step_i][j] for j in range(0, len(steps[step_i])) }
                env_update = int(step_info["env_update"])
                active_modules = { i for i in range(0, num_modules) if modules_active_by_step[step_i][i] > 0 }
                left_match_scores = list(map(float, step_info["0_signal_match_scores"].strip("[]").split(",")))
                right_match_scores = list(map(float, step_info["1_signal_match_scores"].strip("[]").split(",")))
                reg_state = list(map(float, step_info["module_regulator_states"].strip("[]").split(",")))
                # if this is the first step, setup 'previous' state
                if step_i == 0:
                    state_i = 0
                    prev_env_update = env_update
                    prev_active_modules = active_modules
                    prev_match_scores_left = left_match_scores
                    prev_match_scores_right = right_match_scores
                    prev_reg_state = reg_state
                if not found_first_module:
                    prev_active_modules = active_modules
                    found_first_module = len(active_modules) > 0
                # Has anything been repressed/promoted?
                reg_deltas = [reg_state[i] - prev_reg_state[i] for i in range(0, num_modules)]
                promoted_modules = { i for i in range(0, num_modules) if reg_state[i] < prev_reg_state[i] }
                repressed_modules = { i for i in range(0, num_modules) if reg_state[i] > prev_reg_state[i] }
                # promoted_modules = [reg_state[i] < prev_reg_state[i] for i in range(0, num_modules)]
                # repressed_modules = [reg_state[i] > prev_reg_state[i] for i in range(0, num_modules)]

                if ( ( active_modules != prev_active_modules and len(active_modules) != 0 )
                    or (step_i == (len(steps) - 1))
                    or (len(promoted_modules) != 0)
                    or (len(repressed_modules) != 0) ):

                    repressing_calls += len(repressed_modules)
                    promoting_calls += len(promoted_modules)
                    promoted_str = "\"" + str(list(promoted_modules)).replace(" ", "") + "\""
                    repressed_str = "\"" + str(list(repressed_modules)).replace(" ", "") + "\""
                    deltas_str = "\"" + str(reg_deltas).replace(" ", "") + "\""
                    active_modules_str = "\"" + str(list(prev_active_modules)).replace(" ", "") + "\""
                    module_triggered = module_triggered_by_env_cycle[prev_env_update]
                    module_responded = -1 if len(module_response_by_env_cycle[prev_env_update]) == 0 else list(module_response_by_env_cycle[prev_env_update])[0]
                    # reg_graph_fields = ["test_id","test_seq","state_id","env_update","time_step","module_triggered","active_modules","promoted","repressed","reg_deltas"]
                    line_info = {
                        "test_id": trace_test_id,
                        "test_seq": trace_test_seq,
                        "state_id": str(state_i),
                        "env_update": str(prev_env_update),
                        "time_step": str(step_i),
                        "module_triggered": str(module_triggered),
                        "module_responded": str(module_responded),     # module that responds to this env update
                        "active_modules": active_modules_str,
                        "promoted": promoted_str,
                        "repressed": repressed_str,
                        "reg_deltas": deltas_str
                    }
                    reg_lines.append(",".join([line_info[field] for field in reg_graph_fields] ))

                    state_i += 1
                    prev_env_update = env_update
                    prev_active_modules = active_modules
                    prev_match_scores_left = left_match_scores
                    prev_match_scores_right = right_match_scores
                    prev_reg_state = reg_state

        with open(os.path.join(dump_dir, trace_out_name), "w") as fp:
            content = [out_trace_header] + trace_content_lines
            fp.write("\n".join(content))
        print("  Wrote out: ", os.path.join(dump_dir, trace_out_name))

        with open(os.path.join(dump_dir, reg_graph_out_name), "w") as fp:
            fp.write("\n".join(reg_lines))
        print("  Wrote out:", os.path.join(dump_dir, reg_graph_out_name))

        trace_values = [str(promoting_calls), str(repressing_calls)]
        analysis_org_infos.append([run_settings[key] for key in key_settings] + extra_values + trace_values + org)
    # Output analysis org infos
    out_content = list(analysis_header_set)[0] + "\n"
    out_content += "\n".join([",".join(map(str, line)) for line in analysis_org_infos])
    with open(os.path.join(dump_dir, dump_fname), "w") as fp:
        fp.write(out_content)
    print(f" Done! Output written to {os.path.join(dump_dir, dump_fname)}")

if __name__ == "__main__":
    main()