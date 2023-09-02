#!/usr/bin/env python3

import json
import os
import sys

if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <example dir>")
    sys.exit()

pgm = ""
func = ""
with open(sys.argv[1] + "/Makefile") as f:
    for line in f:
        line = line.strip()
        if line.startswith("TARGET"):
            pgm = line.split("=")[-1].strip()
        elif line.startswith("FUNC"):
            func = line.split("=")[-1].strip()

accel_dict = {"pgm": pgm, "func": func}
if pgm.endswith("_tl"):
    accel_dict["addr"] = "0x30000"
srcs = [filename for filename in os.listdir(sys.argv[1]) if filename.endswith(".c") or filename.endswith(".cpp") or filename.endswith(".h")]

for src in srcs:
    with open(sys.argv[1] + "/" + src) as f:
        src_code = f.read()
        # Reformatting src_code in case "int main" is formatted weirdly
        src_code = " ".join(src_code.split())
        if "int main" in src_code:
            accel_dict["srcs"] = [src]
            break

if pgm.endswith("_tl"):
    res = {"TL": [accel_dict]}
else:
    res = {"RoCC": {"custom0": accel_dict}}

with open(sys.argv[1] + "/" + pgm + "_soc.json", "w") as f:
    json.dump(res, f, indent=2)
