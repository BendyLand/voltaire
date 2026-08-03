import sys

all_lines = []
with open("comp") as file:
    for line in file:
        all_lines.append(line)

if len(sys.argv) > 1:
    prefix = sys.argv[1] + '('
    for line in all_lines:
        if prefix in line:
            print(line, end="")
else:
    print("Usage: get_python_functions <function_name>")

