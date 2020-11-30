'''
Script: aggMaxFit.py
For each run, grab the maximum fitness organism at end of run.

'''

import argparse, os, copy, errno, csv, re, sys
import json
import hjson

csv.field_size_limit(sys.maxsize)

run_dir_identifier = "RUN_" # all legit run directories will have this substring in their name

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

def read_csv(file_path):
    content = None
    with open(file_path, "r") as fp:
        content = fp.read().strip().split("\n")
    header = content[0].split(",")
    # header_lu = {header[i].strip():i for i in range(0, len(header))}
    content = content[1:]
    lines = [{header[i]: l[i] for i in range(len(header))} for l in csv.reader(content, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True)]
    return lines

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
        "task",
        "calls_with_promoters",
        "calls_with_repressors"
    ]

    for run in run_dirs:
        print(f"Extracting information from {run}")
        run_config_path = os.path.join(run, "output", "run_config.csv")
        org_analysis_path = find_org_analysis_path(run, update if update >= 0 else None)

        # does the run config file exist?
        if not os.path.exists(run_config_path):
            print(f"Failed to find run parameters ({run_config_path})")
            exit(-1)

        # are there any analysis files?
        if not org_analysis_path:
            print(" >> no analysis files")
            continue

        # find organism analysis file
        if not os.path.exists(org_analysis_path):
            print(f"Failed to find run parameters ({org_analysis_path})")
            exit(-1)

        # Extract organism update from analysis file.
        org_update = org_analysis_path.split(".")[0].split("_")[-1]

        org_trace_path = find_trace_path(run, update if update >= 0 else None)
        trace_update = org_trace_path.split("/")[-1].split("_update_")[-1].split(".")[0]
        if org_update != trace_update:
            print(f"Analysis file and trace file updates do not match: \n  * {org_update}\n  * {trace_update}\n")
            exit(-1)

        # extract run settings
        run_settings = extract_settings(run_config_path)
        # extract task id from the training set file path
        task = run_settings["TRAINING_SET_FILE"].split(".")[0].split("_")[-1]

        ################################################################################################
        # ================================= extract analysis file info =================================
        ################################################################################################
        orgs = read_csv(org_analysis_path)
        org = orgs[-1]
        analysis_header = sorted(org.keys())

        # -- collect extra fields --
        base_score = float(org["aggregate_score"])
        ko_reg_score = float(org["ko_regulation_aggregate_score"])
        ko_gmem_score = float(org["ko_global_memory_aggregate_score"])
        ko_all_score = float(org["ko_all_aggregate_score"])
        ko_up_reg_score = float(org["ko_up_reg_aggregate_score"])
        ko_down_reg_score = float(org["ko_down_reg_aggregate_score"])

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
            "task": task
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
            org_info[field] = org[field]
        for field in config_fields:
            org_info[field] = run_settings[field]
        ################################################################################################


        ################################################################################################
        # ================================= extract trace information =================================
        ################################################################################################
        trace_steps = read_csv(org_trace_path)
        time_steps = [i for i in range(len(trace_steps))]

        # We want to work with the thread state information as a python dictionary, so
        #  raw string ==[hjson]==> OrderedDict ==[json]==> python dictionary
        thread_states = [step["thread_state_info"].replace(",", ",\n") for step in trace_steps]
        thread_states = [list(json.loads(json.dumps(hjson.loads(state)))) for state in thread_states]
        if len(set([len(thread_states), len(time_steps), len(trace_steps)])) != 1:
            print("Trace steps do not match among components.")
            exit(-1)

        # How many test cases do we have in the trace?
        num_testcases = len({step["cur_test_id"] for step in trace_steps})
        # How many modules in the program?
        num_modules = int(org["num_modules"])

        modules_active_ever = set()
        modules_run_by_test = [set() for _ in range(num_testcases)]

        modules_present_by_step = [[0 for m in range(0, num_modules)] for i in range(0, len(time_steps))]
        modules_active_by_step = [[0 for m in range(0, num_modules)] for i in range(0, len(time_steps))]
        # modules_triggered_by_step = [None for i in range(0, len(time_steps))]
        modules_responded_by_step = [None for i in range(0, len(time_steps))]

        # Extract which module responded to each input signal.
        for i in range(0, len(time_steps)):
            step = trace_steps[i]
            cur_response_module_id = int(step["cur_responding_function"])
            # cur_test_case = int(step["cur_test_id"])
            if cur_response_module_id != -1:
                modules_responded_by_step[i] = cur_response_module_id

        # cur_testcase = int(trace_steps[0]["cur_test_id"])
        for i in range(0, len(trace_steps)):
            step_info = trace_steps[i]
            threads = thread_states[i]
            testcase_id = int(step_info["cur_test_id"])
            # Extract what modules are running
            active_modules = []
            present_modules = []
            # any module that responded, is guaranteed to have been running.
            # (but by responding, it would have been removed from threads)
            if modules_responded_by_step[i] != None:
                active_modules.append(int(modules_responded_by_step[i]))
                present_modules.append(int(modules_responded_by_step[i]))
            for thread in threads:
                call_stack = thread["call_stack"]
                # active modules are at the top of the flow stack on the top of the call stack
                active_module = None
                if len(call_stack):
                    if len(call_stack[-1]["flow_stack"]):
                        active_module = call_stack[-1]["flow_stack"][-1]["mp"]
                if active_module != None:
                    active_modules.append(int(active_module))
                    modules_active_ever.add(int(active_module))
                # add ALL modules to present modules
                present_modules += list({flow["mp"] for call in call_stack for flow in call["flow_stack"]})
            # Add present modules for this test case
            for module_id in present_modules: modules_run_by_test[testcase_id].add(int(module_id))
            # Add active modules for this step
            for module_id in active_modules: modules_active_by_step[i][module_id] += 1
            # Add present modules for this step
            for module_id in present_modules: modules_present_by_step[i][module_id] += 1
        ################################################################################################


        ################################################################################################
        # ============================= build instruction execution trace =============================
        ################################################################################################
        exec_trace_out_name = f"trace-exec_update-{update}_run-id-" + run_settings["SEED"] + ".csv"
        exec_trace_orig_fields = ["env_cycle","cpu_step","num_env_states","cur_env_state","cur_response","has_correct_response","num_modules","num_active_threads"]
        exec_trace_fields = exec_trace_orig_fields + ["time_step", "active_instructions"]
        exec_trace_out_lines = [",".join(exec_trace_fields)]
        for step_i in range(0, len(trace_steps)):
            step_info = trace_steps[step_i]
            line_info = {field:step_info[field] for field in exec_trace_orig_fields}
            line_info["time_step"] = step_i
            thread_state = thread_states[step_i]
            executing_instructions = []
            for thread in thread_state:
                if len(thread["call_stack"]) and thread["state"].strip(",") == "running":
                    if len(thread["call_stack"][-1]["flow_stack"]):
                        executing_instructions.append(thread["call_stack"][-1]["flow_stack"][-1]["inst_name"].strip(","))
            line_info["active_instructions"] = f"\"[{','.join(executing_instructions)}]\""
            exec_trace_out_lines.append(",".join([str(line_info[field]) for field in exec_trace_fields]))

        with open(os.path.join(dump_dir, exec_trace_out_name), "w") as fp:
            fp.write("\n".join( exec_trace_out_lines ))
        ################################################################################################

        ################################################################################################
        # ============================== build regulation network graphs ==============================
        ################################################################################################
        reg_graph_out_name = f"reg-graph_update-{update}_run-id-" + run_settings["SEED"] + ".csv"
        reg_graph_fields = ["state_id", "testcase_id", "time_step", "active_modules", "promoted", "repressed" , "reg_deltas"]
        reg_graph_lines = [",".join(reg_graph_fields)]

        # == build env cycle reg graph ==
        state_i = None
        prev_testcase_id = None
        prev_active_modules = None
        prev_reg_state = None
        found_first_module = False

        calls_with_repressors = 0
        calls_with_promotors = 0

        for step_i in range(0, len(trace_steps)):
            step_info = trace_steps[step_i]
            # Get the current test case id
            testcase_id = int(step_info["cur_test_id"])
            # Extract active modules (top of call stacks)
            active_modules = {i for i in range(0, num_modules) if modules_active_by_step[step_i][i] > 0}
            reg_state = list(map(float, step_info["module_regulator_states"].strip("[]").split(",")))
            # if this is the first time step, setup 'previous' state
            if step_i == 0:
                state_i = 0
                prev_testcase_id = testcase_id
                prev_active_modules = active_modules
                prev_reg_state = reg_state
            if not found_first_module:
                prev_active_modules = active_modules
                found_first_module = len(active_modules) > 0

            # Has anything been repressed/promoted?
            reg_deltas = [reg_state[i] - prev_reg_state[i] for i in range(0, num_modules)]
            promoted_modules = { i for i in range(0, num_modules) if reg_state[i] < prev_reg_state[i] }
            repressed_modules = { i for i in range(0, num_modules) if reg_state[i] > prev_reg_state[i] }

            # if current active modules or current env cycle don't match previous, output what happened since last time we output
            if (( active_modules != prev_active_modules and len(active_modules) != 0 )
                or (step_i == (len(trace_steps) - 1))
                or (len(promoted_modules) != 0)
                or (len(repressed_modules) != 0)):

                calls_with_repressors += len(repressed_modules)
                calls_with_promotors += len(promoted_modules)
                promoted_str = "\"" + str(list(promoted_modules)).replace(" ", "") + "\""
                repressed_str = "\"" + str(list(repressed_modules)).replace(" ", "") + "\""
                reg_deltas_str = "\"" + str(reg_deltas).replace(" ", "") + "\""
                active_modules_str = "\"" + str(list(prev_active_modules)).replace(" ", "") + "\"" # active modules _during_ this state

                line_info["state_id"] = state_i
                line_info["testcase_id"] = testcase_id
                line_info["time_step"] = step_i
                line_info["active_modules"] = active_modules_str
                line_info["promoted"] = promoted_str
                line_info["repressed"] = repressed_str
                line_info["reg_deltas"] = reg_deltas_str

                reg_graph_lines.append( ",".join([str(line_info[field]) for field in reg_graph_fields]) )

                prev_active_modules = active_modules
                prev_testcase_id = testcase_id
                prev_reg_state = reg_state
                state_i += 1

        with open(os.path.join(dump_dir, reg_graph_out_name), "w") as fp:
            fp.write("\n".join(reg_graph_lines))
        print("  Wrote out:", os.path.join(dump_dir, reg_graph_out_name))

        ################################################################################################
        org_info["calls_with_promoters"] = calls_with_promotors
        org_info["calls_with_repressors"] = calls_with_repressors

        # append csv line (as a list) for analysis orgs
        analysis_org_infos.append(",".join([str(org_info[field]) for field in fields]))

    # Output analysis org infos
    out_content = list(analysis_header_set)[0] + "\n"
    out_content += "\n".join(analysis_org_infos)
    with open(os.path.join(dump_dir, dump_fname), "w") as fp:
        fp.write(out_content)
    print(f" Done! Output written to {os.path.join(dump_dir, dump_fname)}")

if __name__ == "__main__":
    main()