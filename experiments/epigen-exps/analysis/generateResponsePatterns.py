import argparse, os, copy, errno, csv, re, sys
import numpy as np
from PIL import Image, ImageDraw

IMG_HEIGHT=1000
IMG_WIDTH=1000

csv.field_size_limit(sys.maxsize)

label_settings = ["DEME_WIDTH",
                  "DEME_HEIGHT",
                  "USE_FUNC_REGULATION",
                  "USE_GLOBAL_MEMORY",
                  "NUM_RESPONSE_TYPES",
                  "MAX_RESPONSE_CNT",
                  "DEVELOPMENT_PHASE_CPU_TIME",
                  "MAX_ACTIVE_THREAD_CNT",
                  "SCORE_RESPONSE_TYPE_SPREAD",
                  "EPIGENETIC_INHERITANCE",
                  "score",
                  "CLUMPY_SCORE",
                  "update",
                  "SEED"]
label_shorts = {
    "DEME_WIDTH":"W",
    "DEME_HEIGHT":"H",
    "USE_FUNC_REGULATION":"FReg",
    "USE_GLOBAL_MEMORY":"GMem",
    "NUM_RESPONSE_TYPES":"NumResp",
    "MAX_RESPONSE_CNT":"MaxResp",
    "DEVELOPMENT_PHASE_CPU_TIME":"DevCPU",
    "MAX_ACTIVE_THREAD_CNT":"MaxThreads",
    "SCORE_RESPONSE_TYPE_SPREAD":"ScoreSpread",
    "EPIGENETIC_INHERITANCE":"Epigen",
    "update":"gen"
}


WHITE_HSV=(0, 0, 255)
BLACK_HSV=(0, 0, 0)

response_color_maps = {} # Color maps for responses by NUM_RESPONSE_TYPES

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

def GenerateColorMap(colors):
    if (colors >= 255):
        print("Cannot create a color map with the requested resolution.")
        exit(-1)
    colors = list(np.linspace(0, 300, num=colors, endpoint=True))
    return {i:(int( (colors[i] / 360) * 255), 255, 255) for i in range(0, len(colors))}

def DrawResponses(org_info, out_path):
    # If the out path doesn't exist, make it.
    mkdir_p(out_path)
    # (1) Extract response locations, relevant information for drawing.
    response_locs_str = org_info["response_locs"]
    response_locs = [ [ tuple(map(int, loc.strip("()").split(","))) for loc in resp.strip("[]").split("),") if loc != ""] for resp in response_locs_str.split(";")]
    deme_w = int(org_info["DEME_WIDTH"])
    deme_h = int(org_info["DEME_HEIGHT"])
    num_response_types = int(org_info["NUM_RESPONSE_TYPES"])
    # (2) Generate a map of locations => response ID
    response_map = {}
    for resp_i in range(0, len(response_locs)):
        locs = response_locs[resp_i]
        for loc in locs:
            if loc in response_map:
                print(f"{loc} is already in the response map!")
                exit()
            response_map[loc] = resp_i
    # (3) Generate image to draw on
    image = Image.new(mode="HSV", size=(IMG_WIDTH, IMG_HEIGHT), color=WHITE_HSV)
    # (4) Generate color map (or use existing) for given response count
    if not num_response_types in response_color_maps:
        response_color_maps[num_response_types] = GenerateColorMap(num_response_types)
    color_map = response_color_maps[num_response_types]
    # (5) Draw deme responses
    cell_width = IMG_WIDTH / deme_w
    cell_height = IMG_HEIGHT / deme_h
    draw = ImageDraw.Draw(image)
    for x in range(0, deme_w):
        x_pos = x * cell_width
        for y in range(0, deme_h):
            y_pos = y * cell_height
            cell_resp = response_map[(x,y)] if (x,y) in response_map else None
            cell_color = BLACK_HSV if cell_resp == None else color_map[cell_resp]
            draw.rectangle([(x_pos, y_pos), (x_pos + cell_width, y_pos + cell_height)], fill=cell_color, outline=BLACK_HSV, width=5)
    del draw
    # image.show()
    # () Build file name
    org_info["CLUMPY_SCORE"] = f"{float(org_info['CLUMPY_SCORE']):.5}"
    fname = ";".join([f"{label_shorts[param] if param in label_shorts else param}={org_info[param]}" for param in label_settings]) + ".jpeg"
    image.convert('RGB').save(os.path.join(out_path, fname))
    # exit(-1)

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
    for org in orgs:
        # convert list of org details into key-value map (where parameter name is key and value is value)
        org_dict = {param:org[header_lu[param]] for param in header_lu}
        DrawResponses(org_dict, dump_path)
    print("Done!")





if __name__ == "__main__":
    main()