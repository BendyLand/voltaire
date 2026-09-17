def read_file(path):
    with open(path) as file:
        return file.readlines()


def group_lines(lines):
    groups = []
    current_group = []

    for line in lines:
        if line.startswith("//"):
            if current_group:
                groups.append(current_group)
            current_group = [line]
        else:
            current_group.append(line)

    if current_group:
        groups.append(current_group)

    return groups


def write_lines_to_file(group):
    path = group[0].strip().lstrip("/").strip()
    stubs = [item for item in group[1:] if item.strip()]

    with open(path, "a") as file:
        for item in stubs:
            file.write("\n" + item.strip() + "\n")


stubs = read_file("files")
groups = group_lines(stubs)

for group in groups:
    print(group)
    # write_lines_to_file(group)
