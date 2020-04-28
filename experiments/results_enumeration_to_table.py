from tabulate import tabulate
import json
import re
import os

# table = [["Sun",696000,1989100000],["Earth",6371,5973.6], ["Moon",1737,73.5],["Mars",3390,641.85]]

mode = "release"
# mode = "relwithdebinfo-cygwin"


files = [f for f in os.listdir("../enumeration_tool/cmake-build-" + mode + "/experiments") if os.path.isfile(os.path.join("../enumeration_tool/cmake-build-" + mode + "/experiments", f))]
# print(files)
relevant_files = []
for item in files:
    m = re.search(r'result_([\d]*).txt', item)
    if m is not None:
        relevant_files.append(item)
    pass

print(relevant_files)
last_execution_result = sorted(relevant_files)[-1]
# last_execution_result = sorted(relevant_files)[-2]
print(last_execution_result)

file = last_execution_result
# latest execution before changing the time measurement system: result_1580990981.txt

# file = "maybe_final_result.txt"
# file = "result_1581162020.txt"
# file = "result_1581162225.txt"

# new grammar
# file = "results_parallel_1582056485.txt"

with open(os.path.join("../enumeration_tool/cmake-build-" + mode + "/experiments", file), 'r') as f:
    data = json.load(f)

# print(data)

listed_data = {}

one_gate_funcs = []
no_solution_funcs = []
two_gates_funcs = []

max_time = 0
max_time_function = ""



for item in data:
    if(item["target"] == "2d"):
        print(item["solution"])
        print(item["aiger"])

    if (item["result"] == "solution"):
        m = re.search(r'Current assignment: ([\d, ]*)', item["solution"])
        numbers = re.findall(r'\d+', m.group(1))
        num_gates = item["num_gates"]
        if num_gates == 1:
            one_gate_funcs.append(item["target"])

        if num_gates == 2:
            two_gates_funcs.append(item["target"])

        if num_gates > 4:
            # print(item["target"] + "\n")
            pass

        if num_gates in listed_data:
            listed_data[num_gates]["number"] = listed_data[num_gates]["number"] + 1
            listed_data[num_gates]["total runtime"] = listed_data[num_gates]["total runtime"] + item["time"]
            listed_data[num_gates]["num_formulas"] = listed_data[num_gates]["num_formulas"] + item["num_formulas"]
        else:
            listed_data[num_gates] = {}
            listed_data[num_gates]["number"] = 1
            listed_data[num_gates]["total runtime"] = item["time"]
            listed_data[num_gates]["num_formulas"] = item["num_formulas"]

        if max_time < item["time"]:
            max_time_function = item["target"]
            max_time = item["time"]
    else:
        no_solution_funcs.append(item["target"])

print(two_gates_funcs)
print("One gate:")
print(one_gate_funcs)
print(listed_data)
print(no_solution_funcs)

for key in listed_data:
    listed_data[key]["total runtime"] = listed_data[key]["total runtime"] / 1000000

table2 = []
for key in sorted(listed_data.keys()):
    table2.append([key, listed_data[key]["number"], listed_data[key]["total runtime"]/listed_data[key]["number"], listed_data[key]["num_formulas"]/listed_data[key]["number"]])

table = []
for key in sorted(listed_data.keys()):
    table.append([key, listed_data[key]["number"], listed_data[key]["total runtime"]/listed_data[key]["number"]])


print(max_time)
print(max_time_function)


print(tabulate(table, headers=["#Gates", "#Functions", "Avg. Runtime"], tablefmt="latex"))

print(tabulate(table2, headers=["#Gates", "#Functions", "Avg. Runtime", "Avg. num formulas"], tablefmt="plain"))
