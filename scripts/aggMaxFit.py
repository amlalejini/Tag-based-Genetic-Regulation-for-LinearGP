'''
Script: agg_min_correct_networks.py
For each run, grab smallest correct solution network. If run has none, report none.
'''

import argparse, os, copy, errno, csv, re


key_settings = [
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


'''
def main():
    parser = argparse.ArgumentParser(description="Data aggregation script.")
    parser.add_argument("data_directory", type=str, help="Target experiment directory.")
    parser.add_argument("dump_directory", type=str, help="Where to dump this?")
    parser.add_argument("-u", "--update", type=int, help="max update to look for solutions")

    args = parser.parse_args()

    data_directory = args.data_directory
    dump = args.dump_directory

    print("Pulling smallest network solutions from all runs in {}".format(data_directory))

    mkdir_p(dump)

    # Get a list of all runs
    runs = [d for d in os.listdir(data_directory) if d.strip("PROBLEM_").split("__")[0] in problem_whitelist]
    runs.sort()

    if args.update != None:
        update = args.update
        print("Looking for best solutions from update {} or earlier.".format(update))

        solutions_content = "treatment,run_id,problem,arg_type,arg_mut_rate,tag_rand_rate,mem_searching,solution_found,solution_length,update_found,update_first_solution_found,program\n"

        for run in runs:
            print("Run: {}".format(run))
            run_dir = os.path.join(data_directory, run)
            run_id = run.split("__")[-1]
            run = "__".join(run.split("__")[:-1])
            treatment = run
            run_sols = os.path.join(run_dir, "output", "solutions.csv")

            problem = run.strip("PROBLEM_").split("__")[0]

            arg_type = None
            for thing in arg_types:
                if thing in treatment: arg_type = arg_types[thing]
            if arg_type == None:
                print("Unrecognized arg type! Exiting.")
                exit()

            arg_mut_rate = None
            for thing in mut_rates:
                if thing in treatment: arg_mut_rate = mut_rates[thing]
            if arg_mut_rate == None:
                print("Unrecognized arg mut rate! Exiting.")
                exit()

            tag_rand_rate = None
            for thing in tag_rand_rates:
                if thing in treatment: tag_rand_rate = tag_rand_rates[thing]
            if tag_rand_rate == None:
                tag_rand_rate = "NONE"
                print("Failed to figure out tag randomization rate!")

            run_log_fpath = os.path.join(run_dir, "run.log")
            with open(run_log_fpath, "r") as logfp:
                log_content = logfp.read()

            mem_searching = None
            if "set PROGRAM_ARGUMENTS_TYPE_SEARCH 0" in log_content:
                mem_searching = "0"
            else:
                mem_searching = "1"

            file_content = None
            with open(run_sols, "r") as fp:
                file_content = fp.read().strip().split("\n")

            header = file_content[0].split(",")
            header_lu = {header[i].strip():i for i in range(0, len(header))}
            file_content = file_content[1:]

            solutions = [l for l in csv.reader(file_content, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True)]
            # Add smallest solution to smallest solution doc
            min_program = None
            sol_found = False
            if len(solutions) > 0:
                # Find the smallest program
                for i in range(0, len(solutions)):
                    sol_update = int(solutions[i][header_lu["update"]])
                    if sol_update > update: continue
                    if min_program == None:
                        min_program = i
                        sol_found = True
                    elif float(solutions[i][header_lu["program_len"]]) < float(solutions[min_program][header_lu["program_len"]]):
                        min_program = i
                        sol_found = True

            if sol_found:
                # Record timing info about first solution
                update_first_sol = solutions[0][header_lu["update"]]
                # Record info about smallest solution
                min_sol = solutions[min_program]
                program_len = min_sol[header_lu["program_len"]]
                update_found = min_sol[header_lu["update"]]
                program = min_sol[header_lu["program"]]
            else:
                update_first_sol = "NONE"
                program_len = "NONE"
                update_found = "NONE"
                program = "NONE"
            # "treatment,run_id,problem,uses_cohorts,solution_found,solution_length,update_found,program\n"
            solutions_content += ",".join(map(str,[treatment, run_id, problem, arg_type, arg_mut_rate, tag_rand_rate, mem_searching, sol_found, program_len, update_found, update_first_sol, '"{}"'.format(program)])) + "\n"
        with open(os.path.join(dump, "min_programs__update_{}.csv".format(update)), "w") as fp:
            fp.write(solutions_content)
'''

if __name__ == "__main__":
    extract_settings("./run.log")