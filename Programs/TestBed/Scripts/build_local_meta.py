#!/usr/bin/env python

# 0. call this script form Programs/TestBed folder
# 1. generate json with all files
# 2. build local_meta.db

import os
import sys
sys.path.append(os.path.abspath("../../Programs/SdlcBuilder"))
from pack_datum import generate_meta  # noqa

# collect all files
all_files = []

input_data_dir = 'Data'
output_json_file = 'Scripts/all.json'

for root, dirs, files in os.walk(input_data_dir):
    for file_name in files:
        file_path = os.path.join(root, file_name)
        file_path_from_folder = os.path.normpath(os.path.relpath(file_path, input_data_dir))
        # always use unix / in file path to compare strings values
        all_files.append(file_path_from_folder.replace('\\', '/'))

# print(all_files)

with open(output_json_file, "wb") as f:
    f.write("""
[
    {
        "name": "all",
        "dependencies": [],
        "files": [
""")

    for file_name in all_files:
        f.write('            \"' + file_name)
        if file_name == all_files[-1]:
            f.write('\"\n')
        else:
            f.write('\",\n')

    f.write("""        ]
        }
    ]
""")


generate_meta(input_data_dir, [output_json_file], 'Scripts/exclude.json', input_data_dir + '/local_meta.db')

os.remove(output_json_file)  # uncomment for debug

