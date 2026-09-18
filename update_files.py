def read_file(path):
    lines = []
    with open(path) as file:
        for line in file:
            lines.append(line)
    return lines


def pair_lines(stubs):
    pair = []
    pairs = []
    for line in stubs:
        if line.startswith("// File:"):
            start = line.index(":")
            pair.append(line[start+1:].strip())
        elif line.strip() == "":
            continue
        else:
            pair.append(line.strip())
        if len(pair) == 2:
            pairs.append(pair)
            pair = []
    return pairs


stubs = read_file("stubs")
pairs = pair_lines(stubs)




