import argparse, os, copy, errno, csv, re, sys
import numpy as np

IMG_HEIGHT=1000
IMG_WIDTH=1000

csv.field_size_limit(sys.maxsize)

carry_settings = ["SEED",
                  "DEME_WIDTH",
                  "DEME_HEIGHT",
                  "USE_FUNC_REGULATION",
                  "USE_GLOBAL_MEMORY",
                  "NUM_RESPONSE_TYPES",
                  "MAX_RESPONSE_CNT",
                  "DEVELOPMENT_PHASE_CPU_TIME",
                  "MAX_ACTIVE_THREAD_CNT",
                  "SCORE_RESPONSE_MODE",
                  "EPIGENETIC_INHERITANCE",
                  "score",
                  "update",
                  "relies_on_reg",
                  "relies_on_mem",
                  "relies_on_msg",
                  "relies_on_imprinting",
                  "relies_on_repro_tag"
                  ]

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

post_scripts = ["",
                "__ko_regulation",
                "__ko_global_memory",
                "__ko_messaging",
                "__ko_regulation_imprinting",
                "__ko_reproduction_tag_control"]

def extract_location_info(org_info):

    deme_h = int(org_info["DEME_HEIGHT"])
    deme_w = int(org_info["DEME_WIDTH"])

    by_eval_type = {}
    for post in post_scripts:
        by_eval_type[post] = {"response_map": None, "active_locs": None}
        response_locs_str = org_info["response_locs" + post]
        response_locs = [ [ tuple(map(int, loc.strip("()").split(","))) for loc in resp.strip("[]").split("),") if loc != ""] for resp in response_locs_str.split(";")]
        active_locs_str = org_info["active_cell_locs" + post].strip("[]")
        active_locs = { tuple(map(int, cell.strip("()").split(","))) for cell in active_locs_str.split("),") }
        # Build a response map
        response_map = {}
        for resp_i in range(0, len(response_locs)):
            locs = response_locs[resp_i]
            for loc in locs:
                if loc in response_map:
                    print(f"{loc} is already in the response map!")
                    exit()
                response_map[loc] = resp_i
        by_eval_type[post]["response_map"] = response_map
        by_eval_type[post]["active_locs"] = active_locs

    # Build lines
    lines = []
    for x in range(0, deme_w):
        for y in range(0, deme_h):
            line_info = {"x": x, "y": y}
            for eval_type in by_eval_type:
                pos = (x, y)
                response = -1 if pos not in by_eval_type[eval_type]["response_map"] else by_eval_type[eval_type]["response_map"][(x,y)]
                active = int(pos in by_eval_type[eval_type]["active_locs"])
                line_info["response"+eval_type] = response
                line_info["active"+eval_type] = active
            lines.append(line_info)

    computed_fields = ["x", "y"] + ["response"+eval_type for eval_type in by_eval_type] + ["active"+eval_type for eval_type in by_eval_type]
    out_content = {"header": ",".join(carry_settings + computed_fields), "lines": []}
    # Build output file
    for line in lines:
        out_content["lines"].append(",".join( [org_info[field] for field in carry_settings] + [str(line[field]) for field in computed_fields] ))
    return out_content

def main():
    parser = argparse.ArgumentParser(description="Data aggregation script.")
    parser.add_argument("--data", type=str, help="Where should we pull data (expects .csv file)?")
    parser.add_argument("--dump", type=str, default=".", help="Where should we dump the output from this script?")
    args = parser.parse_args()
    data_path = args.data
    dump_path = args.dump
    # Is the data file for real?
    if not os.path.isfile(data_path):
        print("Unable to locate data file: ", data_path)
        exit(-1)
    # Open data file, extract data.
    content = None
    with open(data_path, "r") as fp:
        content = fp.read().strip().split("\n")
    # Grab header info from content and create a header=>position lookup table
    header = content[0].split(",")
    header_lu = {header[i].strip():i for i in range(0, len(header))}
    # Extract organism data
    content = content[1:]
    orgs = [l for l in csv.reader(content, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True)]
    # Draw each organism's response layout
    out_header = ""
    out_lines = []
    for org in orgs:
        # convert list of org details into key-value map (where parameter name is key and value is value)
        org_dict = {param:org[header_lu[param]] for param in header_lu}
        out_content = extract_location_info(org_dict)
        out_header = out_content["header"]
        out_lines += out_content["lines"]
    out_content = [out_header] + out_lines
    with open(os.path.join(dump_path, "response_patterns.csv"), "w") as fp:
        fp.write( "\n".join(out_content))
    print("Done!")





if __name__ == "__main__":
    main()