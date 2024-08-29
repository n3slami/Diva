import sys

import re


if __name__ == "__main__":
    in_path = sys.argv[1]
    # out_path = sys.argv[2]
    if len(sys.argv) >= 3:
        use_header = sys.argv[2]
    else:
        use_header = False

    data_lines=open(in_path, 'r+').read()

    # the result line regex
    # parse the stripped latency timings
    latency_matches = [mat.split(',')[-1] for mat in [d.strip() for d in re.findall(r"[0-9]+,[0-9]+,[0-9]+,[0-9]+,[0-9]+,[0-9]+", data_lines)]]
    # parse the stripped selectivity values
    sel_matches = [d.strip() for d in re.findall(r"\n[0-9\.]+\n", data_lines) if d.strip != '.']

    # print(latency_matches)
    # print(sel_matches)

    records = zip(latency_matches, sel_matches)

    assert(len(latency_matches) == len(sel_matches))

    # print header
    if use_header: 
        print('selectivity' + "," + 'latency_s')
    for record in records:
        # sel, latency
        print(record[1] + "," + record[0])

    # end