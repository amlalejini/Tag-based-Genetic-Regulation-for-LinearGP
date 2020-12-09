import argparse, os, csv, sys, errno, json

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

def main():
    # Setup the commandline argument parser
    parser = argparse.ArgumentParser(description="Data aggregation script.")
    parser.add_argument("--data", type=str, help="Where should we pull graph data?")
    parser.add_argument("-cats", "--instruction_categories", type=str, help="JSON file describing instruction categories")
    parser.add_argument("-mf", "--max_fit_file", type=str, help="File with max fit organism summary information")
    parser.add_argument("--dump", type=str, help="Where to dump this?", default=".")

    # Extract arguments from commandline
    args = parser.parse_args()
    data_dir = args.data
    dump_dir = args.dump
    max_fit_file_path = args.max_fit_file
    inst_categories_file_path = args.instruction_categories

    if not os.path.isdir(data_dir):
        print("Unable to locate all data.")
        exit(-1)

    if not os.path.isfile(inst_categories_file_path):
        print("Unable to locate instruction categories.")
        exit(-1)

    if not os.path.isfile(max_fit_file_path):
        print("Unable to locate max fit file.")
        exit(-1)

    mkdir_p(dump_dir)

    # find all execution traces
    traces = [fname for fname in os.listdir(data_dir) if "trace-exec" in fname]

    # process max fit file
    max_fit_info = read_csv(max_fit_file_path)
    # - organize max_fit_info by seed
    max_fit_info = {entry["SEED"]:entry for entry in max_fit_info}

    # process instruction categories
    inst_category_info = None
    with open(inst_categories_file_path, "r") as fp:
        inst_category_info = json.load(fp)
    inst_categories = set(inst_category_info["categories"])
    instructions = inst_category_info["instructions"]
    partial_instructions = inst_category_info["partial_instructions"]

    # summarize each trace
    mf_fields = [
        "SEED",
        "matchbin_metric",
        "matchbin_thresh",
        "matchbin_regulator",
        "TAG_LEN",
        "USE_FUNC_REGULATION",
        "USE_GLOBAL_MEMORY",
        "MUT_RATE__INST_ARG_SUB",
        "MUT_RATE__INST_SUB",
        "MUT_RATE__INST_INS",
        "MUT_RATE__INST_DEL",
        "MUT_RATE__SEQ_SLIP",
        "MUT_RATE__FUNC_DUP",
        "MUT_RATE__FUNC_DEL",
        "MUT_RATE__INST_TAG_BF",
        "MUT_RATE__FUNC_TAG_BF",
        "MAX_FUNC_CNT",
        "MAX_FUNC_INST_CNT",
        "MAX_ACTIVE_THREAD_CNT",
        "MAX_THREAD_CAPACITY",
        "INST_MIN_ARG_VAL",
        "INST_MAX_ARG_VAL",
        "update",
        "notation"
    ]
    fields = mf_fields + \
        [f"{category}_inst_cnt" for category in inst_categories] + \
        [f"{category}_inst_prop" for category in inst_categories] + \
        ["total_instructions_executed", "total_execution_time"]

    print(fields)

    lines = [",".join(fields)]
    skip_insts = {'', "none"}
    for trace_i in range(len(traces)):
        trace_path = os.path.join(data_dir, traces[trace_i])
        seed = trace_path.split("run-id-")[-1].replace(".csv", "")
        trace = read_csv(trace_path)
        trace_info = {field: max_fit_info[seed][field] for field in mf_fields}
        category_counts = {category:0 for category in inst_categories}
        total_inst_executed = 0
        total_execution_time = len(trace)
        for step in trace:
            active_instructions = step["active_instructions"].strip("[]").split(",")
            active_instructions = [inst.lower() for inst in active_instructions]
            total_inst_executed += len(active_instructions)
            for inst in active_instructions:
                if inst in skip_insts: continue
                category = ""
                if inst in instructions:
                    category = instructions[inst]
                else:
                    for partial in partial_instructions:
                        if partial in inst: category = partial_instructions[partial]
                category_counts[category] += 1

        trace_info["total_execution_time"] = total_execution_time
        for category in category_counts:
            trace_info[f"{category}_inst_cnt"] = category_counts[category]
            trace_info[f"{category}_inst_prop"] = float(category_counts[category]) / float(total_inst_executed)
        trace_info["total_instructions_executed"] = total_inst_executed

        lines.append(",".join([str(trace_info[field]) for field in fields]))

    exec_trace_summary_out_path = os.path.join(dump_dir, "exec_trace_summary.csv")
    print(f"Writing summary out to {exec_trace_summary_out_path}")
    with open(exec_trace_summary_out_path, "w") as fp:
        fp.write("\n".join(lines))
    print("Done!")



if __name__ == "__main__":
    main()