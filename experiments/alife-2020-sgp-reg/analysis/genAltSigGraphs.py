import argparse, os, csv, sys, errno
import networkx as nx

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

def main():
    # Setup the commandline argument parser
    parser = argparse.ArgumentParser(description="Data aggregation script.")
    parser.add_argument("--data", type=str, help="Where should we pull graph data?")
    parser.add_argument("--dump", type=str, help="Where to dump this?", default=".")
    parser.add_argument("--igraphs", action="store_true", help="Should we output igraph files?")

    # Extract arguments from commandline
    args = parser.parse_args()
    data_dir = args.data
    dump_dir = args.dump
    dump_igraphs = args.igraphs
    print(dump_igraphs)

    if not os.path.isdir(data_dir):
        print("Unable to locate all data.")
        exit(-1)

    mkdir_p(dump_dir)

    # find all reg graphs
    graphs = [fname for fname in os.listdir(data_dir) if "reg-graph" in fname]
    graph_fields = ["run_id", "test_id", "node_cnt","edge_cnt","promoted_edges_cnt","repressed_edges_cnt","non_self_promoting_edges","non_self_repressing_edges","non_self_edges","self_promoting_edges","self_repressing_edges","self_edges","graph_density","total_degree","total_out_degree","total_in_degree","flow_hierarchy","reciprocity"]
    graph_summary_output = [",".join(graph_fields)]
    for graph_file in graphs:
        print("Processing", graph_file)
        run_id = graph_file.strip(".csv").split("run-id-")[-1]
        data_path = os.path.join(data_dir, graph_file)
        # lazy conversion from single test id generation script => generating all test ids
        content = None
        with open(data_path, "r") as fp:
            content = fp.read().strip().split("\n")
        header = content[0].split(",")
        header_lu = {header[i].strip():i for i in range(0, len(header))}
        content = content[1:]
        lines = [l for l in csv.reader(content, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True)]
        nodes = {} # nodes are indexed off of module ids

        promoted_edges = {} # edges are indexed off of frozen tuples
        repressed_edges = {}
        # nodes have the following properties:
        #  - id, times_active, times_repressed, times_promoted
        # edges have the following properties:
        #  - to, from, type (promote, repress), delta

        def GetModIDs(mod_list_str):
            ids = list({mod for mod in mod_list_str.strip("[]").split(",") if mod != ''})
            return ids
        def GenNewNodeDict(node_id = None):
            return {"id": node_id, "times_active": 0, "times_repressed": 0, "times_promoted": 0}
        def GenNewEdgeDict():
            return {"to": None, "from": None, "type": None, "match_delta": 0}

        for line in lines:
            info = {header[i]:line[i] for i in range(0, len(line))}
            active_modules = GetModIDs(info["active_modules"])
            promoted_modules = GetModIDs(info["promoted"])
            repressed_modules = GetModIDs(info["repressed"])
            # reg_deltas = list(map(float, info["reg_deltas"].strip("[]").split(",")))
            match_deltas = list(map(float, info["match_deltas"].strip("[]").split(",")))
            # Update nodes
            for mod_id in active_modules:
                if mod_id not in nodes:
                    nodes[mod_id] = GenNewNodeDict(mod_id)
                nodes[mod_id]["times_active"] += 1
            for mod_id in repressed_modules:
                if mod_id not in nodes:
                    nodes[mod_id] = GenNewNodeDict(mod_id)
                nodes[mod_id]["times_repressed"] += 1
            for mod_id in promoted_modules:
                if mod_id not in nodes:
                    nodes[mod_id] = GenNewNodeDict(mod_id)
                nodes[mod_id]["times_promoted"] += 1
            # Update edges
            for active_id in active_modules:
                # update all repressed edges
                for repressed_id in repressed_modules:
                    edge = (active_id, repressed_id)
                    if edge not in repressed_edges:
                        repressed_edges[edge] = GenNewEdgeDict()
                        repressed_edges[edge]["to"] = repressed_id
                        repressed_edges[edge]["from"] = active_id
                        repressed_edges[edge]["type"] = "repress"
                    edge_delta = match_deltas[int(repressed_id)]
                    repressed_edges[edge]["match_delta"] += edge_delta
                # update all promoted edges
                for promoted_id in promoted_modules:
                    edge = (active_id, promoted_id)
                    if edge not in promoted_edges:
                        promoted_edges[edge] = GenNewEdgeDict()
                        promoted_edges[edge]["to"] = promoted_id
                        promoted_edges[edge]["from"] = active_id
                        promoted_edges[edge]["type"] = "promote"
                    edge_delta = match_deltas[int(promoted_id)]
                    promoted_edges[edge]["match_delta"] += edge_delta
        # Write out the nodes file
        if dump_igraphs:
            nodes_fields = ["id", "times_active", "times_repressed","times_promoted"]
            edges_fields = ["from", "to", "type", "match_delta"]

            nodes_content = [",".join(nodes_fields)] + [",".join([str(nodes[node][field]) for field in nodes_fields]) for node in nodes]
            edges_content = [",".join(edges_fields)]
            edges_content += [",".join([str(promoted_edges[edge][field]) for field in edges_fields]) for edge in promoted_edges]
            edges_content += [",".join([str(repressed_edges[edge][field]) for field in edges_fields]) for edge in repressed_edges]

            nodes_out_path = os.path.join(dump_dir, f"reg_graph_id-{run_id}_nodes.csv")
            edges_out_path = os.path.join(dump_dir, f"reg_graph_id-{run_id}_edges.csv")
            with open(nodes_out_path, "w") as fp:
                fp.write("\n".join(nodes_content))
            with open(edges_out_path, "w") as fp:
                fp.write("\n".join(edges_content))
        # Analyze graph properties.
        #repressed_edges[edge] = GenNewEdgeDict()
        #promoted_edges[edge] = GenNewEdgeDict()
        #nodes
        graph_info = {}
        graph_info["run_id"] = run_id
        graph_info["test_id"] = 0
        graph_info["node_cnt"] = len(nodes)
        graph_info["edge_cnt"] = len(repressed_edges) + len(promoted_edges)
        graph_info["promoted_edges_cnt"] = len(promoted_edges)
        graph_info["repressed_edges_cnt"] = len(repressed_edges)
        graph_info["non_self_promoting_edges"] = sum([1 for edge in promoted_edges if edge[0] != edge[1]])
        graph_info["non_self_repressing_edges"] = sum([1 for edge in repressed_edges if edge[0] != edge[1]])
        graph_info["non_self_edges"] = graph_info["non_self_promoting_edges"] + graph_info["non_self_repressing_edges"]
        graph_info["self_promoting_edges"] = sum([1 for edge in promoted_edges if edge[0] == edge[1]])
        graph_info["self_repressing_edges"] = sum([1 for edge in repressed_edges if edge[0] == edge[1]])
        graph_info["self_edges"] = graph_info["self_repressing_edges"] + graph_info["self_promoting_edges"]
        # Build a directed network x graph representing this network (repressed and promoted relationships)
        graph = nx.DiGraph()
        for edge in promoted_edges:
            graph.add_edge(edge[0], edge[1], attr_dict = {"type": "promote"})
        for edge in repressed_edges:
            graph.add_edge(edge[0], edge[1], attr_dict = {"type": "repress"})
        graph_info["graph_density"] = nx.density(graph)
        graph_info["total_degree"] = sum([degree[1] for degree in nx.degree(graph)])
        graph_info["total_out_degree"] = sum([graph.out_degree(node) for node in graph.nodes])
        graph_info["total_in_degree"] = sum([graph.in_degree(node) for node in graph.nodes])
        try:
            graph_info["flow_hierarchy"] = nx.flow_hierarchy(graph)
        except:
            graph_info["flow_hierarchy"] = "NA"
        graph_info["reciprocity"] = nx.overall_reciprocity(graph) if len(graph.nodes) else "NA"
        line = ",".join([str(graph_info[field]) for field in graph_fields])
        graph_summary_output.append(line)
    graph_out_name = os.path.join(dump_dir, "reg_graphs_summary.csv")
    with open(graph_out_name, "w") as fp:
        fp.write("\n".join(graph_summary_output))


if __name__ == "__main__":
    main()