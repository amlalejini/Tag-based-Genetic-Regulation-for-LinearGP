'''
Generate signal-response reprogramming input-output examples
'''
import argparse

def main():
    parser = argparse.ArgumentParser(description="Generate input/output examples for the contextual-signal problem.")
    parser.add_argument("--signals", type=int, help="How many input signal types are there?")

    args = parser.parse_args()
    num_signals = args.signals

    # input,output,type
    # OP:S1;OP:S1,0,S1S1

    signals = [f"S{i}" for i in range(num_signals)]
    responses = [f"{i}" for i in range(num_signals)]

    # Each signal will be used to provide context for the correct response to the next signal.
    examples = []
    resp_modifier = 0
    for i in range(len(signals)):
        context = signals[i]
        for k in range(len(signals)):
            resp_sig = signals[k]
            response = responses[(k + resp_modifier)%len(responses)]
            examples.append(
                {"input": f"OP:{context};OP:{resp_sig}",
                 "output": f"{response}",
                 "type": f"{context};{resp_sig}"
                }
            )
        resp_modifier += 1

    fields = ["input","output","type"]
    lines = [",".join(fields)]
    for example in examples:
        lines.append(",".join([example[key] for key in fields]))
    with open(f"examples_S{num_signals}.csv", "w") as fp:
        fp.write("\n".join(lines))




if __name__ == "__main__":
    main()